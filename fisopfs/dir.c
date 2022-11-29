#define _GNU_SOURCE
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <linux/limits.h>
#include "inode.h"
#include "dir.h"

#define ALL_PERMISSIONS (S_IRWXU | S_IRWXG | S_IRWXO)

/*
 * Returns the dir_entry at offset through the dir_entry_dest parameter.
 * If offset points to EOF, dir_entry_dest->name[0] == '\0', and dir_entry_dest->inode_id == -1
 * Upon error returns the error code.
 */
int read_directory(inode_t *dir, size_t offset, dir_entry_t *dir_entry_dest) {
	ssize_t ret_val = inode_read((char *) dir_entry_dest, sizeof(dir_entry_t), dir, offset);
	if (ret_val != sizeof(dir_entry_t)) {
		dir_entry_dest->name[0] = '\0';
		dir_entry_dest->inode_id = -1;
		return ret_val >= 0 ? -1 : -ret_val; // if inode_read didn't return an error, return -1
	}
	return EXIT_SUCCESS;
}

/*
 * Given a dir and a relative path, returns the inode_id for the file/directory targeted by path
 * If none is found, returns -1.
 */
int _get_iid_from_path(superblock_t *superblock, inode_t *dir, char *path) {
	if (path[0] == '\0') // path was '/' terminated, return the dir
		return dir->stats.st_ino;
	char *slash_pos = strchrnul(path, '/');
	bool end_of_path = !*slash_pos; // true if slash_pos points to \0, false otherwise
	*slash_pos = '\0';

	size_t offset = 0;
	dir_entry_t dir_entry;
	inode_t *subdir;
	read_directory(dir, offset, &dir_entry);
	while (dir_entry.inode_id >= 0) { // inode_id < 0 indicates invalid inode
		offset += sizeof(dir_entry_t);
		if (strncmp(dir_entry.name, path, MAX_FILENAME_LENGTH) != 0) {// the path doesn't match
			read_directory(dir, offset, &dir_entry);
			continue;
		}
		if (end_of_path) // end of path, file or dir was found
			return dir_entry.inode_id;

		// expecting a dir to continue the search
		get_inode_from_iid(superblock, dir_entry.inode_id, &subdir);
		if (S_ISDIR(subdir->stats.st_mode)) {
			return _get_iid_from_path(superblock, subdir, slash_pos + 1);
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
int get_iid_from_path(superblock_t *superblock, const char *path) {
	if (path[0] != '/')
		return -EINVAL;
	char path_copy[PATH_MAX];
	strcpy(path_copy, path);
	return _get_iid_from_path(superblock, &superblock->inode_tables[0]->inodes[0], path_copy + 1);
}

static void init_link(dir_entry_t *dir_entry, int target_inode_id, const char *target_name) {
	strcpy(dir_entry->name, target_name);
	dir_entry->inode_id = target_inode_id;
}

/*
 * ret_val is negative upon error
 */
int create_dir_entry(inode_t *parent_dir, int entry_inode_id, const char *name) {
	dir_entry_t dir_entry = {
		.inode_id = entry_inode_id
	};
	strcpy(dir_entry.name, name);
	ssize_t ret_val = inode_write((char *) &dir_entry, sizeof(dir_entry_t), parent_dir, parent_dir->stats.st_size);
	return ret_val > 0 ? EXIT_SUCCESS : ret_val;
}

/*
 * Does not set the inode id inside the inode struct, must be done externally
 *  ret_val is negative upon error, else EXIT_SUCCESS
 */
int init_dir(inode_t *dir, int inode_id, int parent_inode_id) {
	
	// set stats
	struct stat *dir_st = &dir->stats;
	dir_st->st_nlink = 2; // one from parent and another one from '.'
	dir_st->st_mode = S_IFDIR | ALL_PERMISSIONS; // podríamos usar umask para ver los default
	dir_st->st_uid = 1000; // CORREGIR: deberíamos setearlo al usuario actual (getcuruid? algo así)
	dir_st->st_gid = 1000; // CORREGIR: deberíamos setearlo al grupo actual
	time(&dir_st->st_mtime);
	time(&dir_st->st_ctime);
	time(&dir_st->st_atime);
	dir_st->st_blocks = 1;
	
	// set dir_entries
	int ret_val = create_dir_entry(dir, inode_id, ".");
	if (ret_val != EXIT_SUCCESS)
		return ret_val;
	ret_val = create_dir_entry(dir, parent_inode_id, "..");
	return ret_val;
}

/*
 * Returns dir's inode_id or negative error.
 */
int create_dir(superblock_t *superblock, const char *name, int parent_inode_id) {
	inode_t *dir = malloc_inode(superblock);
	if (!dir)
		return ENOMEM;
	int dir_inode_id = dir->stats.st_ino;
	int ret_val = init_dir(dir, dir_inode_id, parent_inode_id);
	if (ret_val != EXIT_SUCCESS) {
		free_inode(superblock, dir_inode_id);
		return ret_val;
	}
	// increase parent's link count, and add self to parent's dir_entries
	inode_t *parent_dir;
	if (get_inode_from_iid(superblock, parent_inode_id, &parent_dir) == EXIT_SUCCESS
		&& dir_inode_id != ROOT_DIR_INODE_ID) {
			create_dir_entry(parent_dir, dir_inode_id, name);
			++parent_dir->stats.st_nlink;
	}
	return dir->stats.st_ino;
}
