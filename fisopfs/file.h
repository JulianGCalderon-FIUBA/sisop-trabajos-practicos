#ifndef FILE_H
#define FILE_H
#include "inode.h"
#include <fuse.h>


/*
 * Returns file's inode_id or negative error.
 */
int create_file(superblock_t *superblock,
                const char *name,
                int parent_inode_id,
                mode_t mode,
                struct fuse_file_info *file_info);

#endif  // FILE_H
