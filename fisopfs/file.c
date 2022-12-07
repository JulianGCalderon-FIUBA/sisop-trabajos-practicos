#define _DEFAULT_SOURCE  // for S_IFREG
#include <fuse.h>        // for struct fuse_file_info
#include "inode.h"
#include "dir.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

/*
 * Returns file's inode_id or negative error.
 */
int
create_file(superblock_t *superblock,
            const char *name,
            int parent_inode_id,
            mode_t mode,
            struct fuse_file_info *file_info)
{
	// ignore struct fuse_file_info *file_info
	inode_t *file = malloc_inode(superblock);
	if (!file)
		return -ENOMEM;

	int file_inode_id = file->stats.st_ino;
	file->stats.st_mode = S_IFREG | mode;

	// add self to parent's dir_entries
	inode_t *parent_dir;
	if (get_inode_from_iid(superblock, parent_inode_id, &parent_dir) ==
	    EXIT_SUCCESS) {
		create_dir_entry(superblock, parent_dir, file_inode_id, name);
	}
	return file->stats.st_ino;
}
