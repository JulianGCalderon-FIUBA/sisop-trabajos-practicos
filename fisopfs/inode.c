#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <linux/limits.h>
#include "bitmap.h"
#include "inode.h"

#define ALL_PERMISSIONS (S_IRWXU | S_IRWXG | S_IRWXO)

/*
 * Returns the dir_entry at offset within the dir directory.
 */
dir_entry_t *read_directory(inode_t *dir, size_t offset) {
	int page_num = offset / PAGE_SIZE;
	if (page_num >= dir->stats.st_blocks)
		return NULL;

	offset &= PAGE_SIZE - 1; // equivalent to the % operator when PAGE_SIZE is a power of 2
	char *dir_entries = (char *) dir->pages[page_num];
	return (dir_entry_t *) (dir_entries + offset); // if offset pointed to invalid dir_entry, dir_entry->name[0] == '\0'
}
// CORREGIR: no me gusta que pueda devolver dos cosas distintas en caso de error (NULL o una dir_entry inválida)


/*
 * 
 */
int get_inode_by_id(superblock_t *superblock, int inode_id, inode_t **inode_dest) {
	puts("[debug] get_inode_by_id");
	if (inode_id < 0)
		return EINVAL;

	int table_num = inode_id / INODES_PER_TABLE;
	if (bitmap_getbit(&superblock->free_tables_bitmap, table_num) != 0) { // table is free and therefore empty 
		*inode_dest = NULL;
		return ENOENT;
	}
	int inode_pos = inode_id % INODES_PER_TABLE;
	if (bitmap_getbit(&superblock->inode_tables[table_num]->free_inodes_bitmap, inode_pos) != 0) { // inode is free
		*inode_dest = NULL;
		return ENOENT;
	}

	*inode_dest = &superblock->inode_tables[table_num]->inodes[inode_pos];
	return EXIT_SUCCESS;
}

/*
 * Given a dir and a relative path, returns the inode_id for the file/directory targeted by path
 * If none is found, returns -1.
 */
int _get_inode_id(superblock_t *superblock, inode_t *dir, char *path) {
	if (path[0] == '\0') // path was '/' terminated, return the dir
		return dir->stats.st_ino;
	char *slash_pos = strchrnul(path, '/');
	bool end_of_path = !*slash_pos; // true if slash_pos points to \0, false otherwise
	*slash_pos = '\0';

	size_t offset = 0;
	dir_entry_t *dir_entry;
	inode_t *subdir;
	dir_entry = read_directory(dir, offset);
	while (dir_entry->name[0]) { // name starts with '\0' if dir_entry is not valid
		offset += sizeof(dir_entry_t);
		if (strncmp(dir_entry->name, path, MAX_FILENAME_LENGTH) != 0) {// the path doesn't match
			dir_entry = read_directory(dir, offset);
			continue;
		}
		if (end_of_path) // end of path, file or dir was found
			return dir_entry->inode_id;

		// expecting a dir to continue the search
		get_inode_by_id(superblock, dir_entry->inode_id, &subdir);
		if (S_ISDIR(subdir->stats.st_mode)) {
			return _get_inode_id(superblock, subdir, slash_pos + 1);
		}
		break; // expected a directory (could be a parent dir) but a file was found instead
	}
	return -ENOENT; // ERROR: No such file or directory
}

/*
 * Given an absolute path, returns the inode_id for the file/directory targeted by path
 * Upon error, returns a negated error code
 * path must start with /
 */
int get_inode_id(superblock_t *superblock, const char *path) {
	if (path[0] != '/')
		return -EINVAL;
	char path_copy[PATH_MAX];
	strcpy(path_copy, path);
	return _get_inode_id(superblock, &superblock->inode_tables[0]->inodes[0], path_copy + 1);
}


static void init_link(dir_entry_t *dir_entry, int target_inode_id, const char *target_name) {
	strcpy(dir_entry->name, target_name);
	dir_entry->inode_id = target_inode_id;
}

int init_dir(inode_t *dir, int inode_id, int parent_inode_id) {
	// set stats
	struct stat *dir_st = &dir->stats;
	dir_st->st_nlink = 2; // one from parent and another one from '.'
	dir_st->st_mode = S_IFDIR | ALL_PERMISSIONS;
	dir_st->st_uid = 1000;
	dir_st->st_gid = 1000;
	dir_st->st_size = PAGE_SIZE;
	time(&dir->stat_crtime);
	time(&dir_st->st_mtime);
	time(&dir_st->st_ctime);
	time(&dir_st->st_atime);
	dir_st->st_blocks = 1;

	// alloc first inode page
	dir_entry_t *dir_entries = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (dir_entries == NULL)
		return ENOMEM;
	dir->pages[0] = (char *) dir_entries;
	for (int page_num = 1; page_num < PAGES_PER_INODE; ++page_num)
		dir->pages[page_num] = NULL;
	
	// set dir_entries
	init_link(&dir_entries[0], inode_id, ".");
	init_link(&dir_entries[1], parent_inode_id, "..");
	for (int dir_entry_num = 2; dir_entry_num < DIR_ENTRIES_PER_PAGE; ++dir_entry_num)
		dir_entries[dir_entry_num].name[0] = '\0'; // mark as empty
	read_directory((inode_t *)dir, 0);
	read_directory((inode_t *)dir, sizeof(dir_entry_t));
	read_directory((inode_t *)dir, 2*sizeof(dir_entry_t));
	return EXIT_SUCCESS;
}

/*
 * 
 */
int init_filesystem(superblock_t *superblock) {
	puts("[debug] init_filesystem");
	bitmap_set_all_1(&superblock->free_tables_bitmap); // mark all tables as free/unused
	bitmap_clearbit(&superblock->free_tables_bitmap, 0); // ...except for the first table
	
	// request memory for the first inode table
	inode_table_t *inode_table = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (inode_table == NULL)
		return ENOMEM;
	
	// initialise superblock, inode_table and root_dir
	superblock->inode_tables[0] = inode_table;
	bitmap_set_all_1(&inode_table->free_inodes_bitmap);
	bitmap_clearbit(&inode_table->free_inodes_bitmap, 0);
	inode_t *root_dir = &inode_table->inodes[0];
	int root_inode_id = 0; // easier to read
	if (init_dir(root_dir, root_inode_id, root_inode_id) != EXIT_SUCCESS)
		return ENOMEM;
	root_dir->stats.st_ino = root_inode_id;
	return EXIT_SUCCESS;
}
