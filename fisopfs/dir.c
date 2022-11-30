#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include "inode.h"
#include "dir.h"

/*
 * Splits the path on the last forward slash.
 * Writes parent's dir path (including trailing slash) in the second argument,
 * and returns pointer to the character that followed the last slash (the child's name)
 */
char *split_path(const char *path, char *parent_dir_path) {
	char *last_slash_pos = strrchr(path, '/');
	// strncpy so that parent_dir_path contains the trailling slash
	memcpy(parent_dir_path, path, last_slash_pos - path + 1);
	parent_dir_path[last_slash_pos - path + 1] = '\0';
	return last_slash_pos + 1;
}

/*
 * Returns the dir_entry at offset through the dir_entry_dest parameter.
 * If offset points to EOF, dir_entry_dest->name[0] == '\0', and
 * dir_entry_dest->inode_id == -1 Upon error returns the error code.
 */
int
read_directory(inode_t *dir, size_t offset, dir_entry_t *dir_entry_dest)
{
	ssize_t ret_val = inode_read(
	        (char *) dir_entry_dest, sizeof(dir_entry_t), dir, offset);

	if (ret_val != sizeof(dir_entry_t)) {
		dir_entry_dest->name[0] = '\0';
		dir_entry_dest->inode_id = -1;
		return ret_val >= 0
		               ? -1
		               : -ret_val;  // if inode_read didn't return an error (partial read), return -1 (generic error)
	}
	return EXIT_SUCCESS;
}

/*
 * Given a dir and a relative path, returns the inode_id for the file/directory
 * targeted by path If none is found, returns -1.
 */
int
_get_iid_from_path(superblock_t *superblock, inode_t *dir, char *path)
{
	if (path[0] == '\0')  // path was '/' terminated, return the dir
		return dir->stats.st_ino;

	char *slash_pos = strchrnul(path, '/');
	bool end_of_path =
	        !*slash_pos;  // true if slash_pos points to \0, false otherwise
	*slash_pos = '\0';

	size_t offset = 0;
	dir_entry_t dir_entry;
	inode_t *subdir;
	read_directory(dir, offset, &dir_entry);
	while (dir_entry.inode_id >= 0) {  // inode_id < 0 indicates invalid inode
		offset += sizeof(dir_entry_t);
		if (strncmp(dir_entry.name, path, MAX_FILENAME_LENGTH) !=
		    0) {  // the path doesn't match
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
		break;  // expected a directory (could be a parent dir) but a file was found instead
	}
	return -ENOENT;  // ERROR: No such file or directory
}

/*
 * Given an absolute path, returns the inode_id for the file/directory targeted
 * by path Upon error, returns a negated error code path must start with /
 */
int
get_iid_from_path(superblock_t *superblock, const char *path)
{
	if (path[0] != '/')
		return -EINVAL;

	char path_copy[PATH_MAX];
	strcpy(path_copy, path);

	return _get_iid_from_path(superblock,
	                          &superblock->inode_tables[0]->inodes[0],
	                          path_copy + 1);
}

/*
 * ret_val is negative upon error
 */
int
create_dir_entry(inode_t *parent_dir, int entry_inode_id, const char *name)
{
	dir_entry_t dir_entry = { .inode_id = entry_inode_id };
	strcpy(dir_entry.name, name);

	ssize_t ret_val = inode_write((char *) &dir_entry,
	                              sizeof(dir_entry_t),
	                              parent_dir,
	                              parent_dir->stats.st_size);

	return ret_val > 0 ? EXIT_SUCCESS : ret_val;
}

/*
 * Remove a dir_entry, freeing the inode
 */
int
unlink_dir_entry(superblock_t *superblock, inode_t *dir, const char *name)
{
	// find dir_entry
	dir_entry_t deleted_dir_entry;
	size_t deleted_dirent_off = 0;
	while(read_directory(dir, deleted_dirent_off, &deleted_dir_entry) == EXIT_SUCCESS) {
		if (!strcmp(deleted_dir_entry.name, name))
			break;
		deleted_dirent_off += sizeof(dir_entry_t);
	}
	// check that dir_entry was found
	if (strcmp(deleted_dir_entry.name, name))
		return ENOENT;

	dir_entry_t last_dir_entry;
	read_directory(dir, dir->stats.st_size - sizeof(dir_entry_t), &last_dir_entry);
	
	// move last dir_entry to dir_entry position
	inode_write((char *) &last_dir_entry,
			sizeof(dir_entry_t),
			dir,
			deleted_dirent_off);

	// mark last dir_entry as invalid
	last_dir_entry.name[0] = '\0';
	last_dir_entry.inode_id = -1;
	inode_write((char *) &last_dir_entry,
			sizeof(dir_entry_t),
			dir,
			dir->stats.st_size - sizeof(dir_entry_t));

	// delete last dir_entry
	inode_truncate(dir, sizeof(dir_entry_t));
	// free inode
	free_inode(superblock, deleted_dir_entry.inode_id);
	return EXIT_SUCCESS;
}

/*
 * Does not set the inode id inside the inode struct, must be done previously
 *  ret_val is negative upon error, else EXIT_SUCCESS
 */
int
init_dir(inode_t *dir, int parent_inode_id, mode_t mode)
{
	// one link from parent and another one from '.'
	dir->stats.st_nlink = 2;
	dir->stats.st_mode = S_IFDIR | mode;
	// set dir_entries
	int ret_val = create_dir_entry(dir, dir->stats.st_ino, ".");
	if (ret_val != EXIT_SUCCESS)
		return ret_val;

	return create_dir_entry(dir, dir->stats.st_ino, "..");
}

/*
 * Returns dir's inode_id or negative error.
 */
int
create_dir(superblock_t *superblock, const char *name, int parent_inode_id, mode_t mode)
{
	inode_t *dir = malloc_inode(superblock);
	if (!dir)
		return ENOMEM;

	int dir_inode_id = dir->stats.st_ino;
	int ret_val = init_dir(dir, parent_inode_id, mode);
	if (ret_val != EXIT_SUCCESS) {
		free_inode(superblock, dir_inode_id);
		return ret_val;
	}
	// increase parent's link count, and add self to parent's dir_entries
	inode_t *parent_dir;


	if (get_inode_from_iid(superblock, parent_inode_id, &parent_dir) ==
	            EXIT_SUCCESS &&
	    dir_inode_id != ROOT_DIR_INODE_ID) {
		create_dir_entry(parent_dir, dir_inode_id, name);
		++parent_dir->stats.st_nlink;
	}

	return dir->stats.st_ino;
}
