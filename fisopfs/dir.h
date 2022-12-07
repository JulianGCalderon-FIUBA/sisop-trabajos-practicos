#ifndef DIR_H
#define DIR_H

#define ROOT_DIR_INODE_ID 0

#include "inode.h"

#include <sys/types.h>

/*
 * Returns the dir_entry at offset through the dir_entry_dest parameter.
 * If offset points to EOF, dir_entry_dest->name[0] == '\0', and
 * dir_entry_dest->inode_id == -1 Upon error returns the error code.
 */
int read_directory(inode_t *dir, size_t offset, dir_entry_t *dir_entry_dest);

/*
 * Sets the inode id inside the inode struct
 */
int create_dir(superblock_t *superblock,
               const char *name,
               int parent_inode_id,
               mode_t mode);


/*
 * ret_val is negative upon error
 */
int create_dir_entry(superblock_t *superblock,
                     inode_t *parent_dir,
                     int entry_inode_id,
                     const char *name);

/*
 * Returns negative value upon failure
 */
int get_iid_from_path(superblock_t *superblock, const char *path);

/*
 *
 */
int unlink_dir_entry(superblock_t *superblock, inode_t *dir, const char *name);

/*
 * Splits the path on the last forward slash.
 * Writes parent's dir path (including trailing slash) in the argument,
 * and returns pointer to the character that followed the last slash (the child's name)
 */
char *split_path(const char *path, char *parent_dir_path);

#endif  // DIR_H
