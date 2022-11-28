#ifndef DIR_H
#define DIR_H

#define ROOT_DIR_INODE_ID 0
#include "inode.h"

/*
 * Returns the dir_entry at offset through the dir_entry_dest parameter.
 * If offset points to EOF, dir_entry_dest->name[0] == '\0', and dir_entry_dest->inode_id == -1
 * Upon error returns the error code.
 */
int read_directory(inode_t *dir, size_t offset, dir_entry_t *dir_entry_dest);

/*
 * Sets the inode id inside the inode struct
 */
int create_dir(superblock_t *superblock, const char *name, int parent_inode_id); // consider changing name + parent_id -> path

/*
 * 
 */
int get_iid_from_path(superblock_t *superblock, const char *path);

#endif // DIR_H