#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include "bitmap.h"
#include "inode.h"

#define ALL_PERMISSIONS (S_IRWXU | S_IRWXG | S_IRWXO)

/*
 * Returns the dir_entry at offset within the dir directory.
 */
dir_entry_t *read_directory(inode_t dir, size_t offset) {
	puts("[debug] read_directory not implemented");
	return 0;
}


/*
 * 
 */
int get_inode_by_id(superblock_t *superblock, int inode_id, inode_t **inode) {
	puts("[debug] get_inode_by_id not implemented");
	return -1;
}

/*
 * Given a dir and a relative path, returns the inode_id for the file/directory targeted by path
 * If none is found, returns -1.
 */
int _get_inode_id(superblock_t *superblock, inode_t dir, char *path) {
	char *slash_pos = strchrnul(path, '/');
	bool end_of_path = !*slash_pos; // true if slash_pos points to \0, false otherwise
	*slash_pos = '\0';

	size_t offset = 0;
	dir_entry_t *dir_entry;
	inode_t subdir;
	inode_t *subdir_p = &subdir;
	while ((dir_entry = read_directory(dir, offset))) {
		offset += sizeof(dir_entry_t);
		if (strncmp(dir_entry->name, path, MAX_FILENAME_LENGTH) != 0) // the path doesn't match
			continue;

		if (end_of_path) // end of path, file or dir was found
			return dir_entry->inode_id;

		get_inode_by_id(superblock, dir_entry->inode_id, &subdir_p);
		if (!(slash_pos + 1) && S_ISDIR(subdir.stats.st_mode)) // path had trailing '/' and found a dir
			return dir_entry->inode_id;

		// found a parent dir
		if (S_ISDIR(subdir.stats.st_mode)) {
			return _get_inode_id(superblock, subdir, slash_pos + 1);
		}
		break; // expected a directory (could be a parent dir) but a file was found instead
	}
	return -1; // ERROR: No such file or directory
}

/*
 * Given an absolute path, returns the inode_id for the file/directory targeted by path
 * If none is found, returns -1.
 * path must start with /
 */
int get_inode_id(superblock_t *superblock, char *path) {
	return _get_inode_id(superblock, superblock->inode_tables[0]->inodes[0], path + 1);
}

/*
 * 
 */
int create_inode(superblock_t *superblock); // definir args

/*
 * 
 */
int delete_inode(superblock_t *superblock, int inode_id); // si el inodo es un dir es recursivo o da error?

void init_dir_stats(struct stat *dir_st) {
	dir_st->st_nlink = 2; // one from parent and another one from '.'
	dir_st->st_mode = S_IFDIR | ALL_PERMISSIONS;
	dir_st->st_uid = 1000;
	dir_st->st_gid = 1000;
	dir_st->st_size = 0;
	time(&dir_st->st_btime);
	time(&dir_st->st_mtime);
	time(&dir_st->st_ctime);
	time(&dir_st->st_atime);
	dir_st->st_blocks = 0;
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
		return -1;
	
	// initialise superblock, inode_table and root_dir
	superblock->inode_tables[0] = inode_table;
	bitmap_set_all_1(&inode_table->free_inodes_bitmap);
	bitmap_clearbit(&inode_table->free_inodes_bitmap, 0);
	struct stat *root_dir_st = &inode_table->inodes[0].stats;
	init_dir_stats(root_dir_st);
	root_dir_st->st_ino = 0;
	return 0;
}
