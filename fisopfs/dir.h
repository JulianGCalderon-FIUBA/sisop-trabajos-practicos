#ifndef DIR_H
#define DIR_H

#define ROOT_DIR_INODE_ID 0
#include "inode.h"

/*
 * Returns the dir_entry at offset within the dir directory.
 */
dir_entry_t *read_directory(inode_t *dir, size_t offset);

/*
 * Sets the inode id inside the inode struct
 */
int create_dir(superblock_t *superblock, const char *name, int parent_inode_id); // consider changing name + parent_id -> path

/*
 * 
 */
int get_iid_from_path(superblock_t *superblock, const char *path);

#endif // DIR_H