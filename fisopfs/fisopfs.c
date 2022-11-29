#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <linux/limits.h>
#include "bitmap.h"
#include "inode.h"
#include "dir.h"

superblock_t superblock;

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr(%s)\n", path);
	int inode_id;
	inode_t *inode;

	if ((inode_id = get_iid_from_path(&superblock, path)) < 0)
		return -ENOENT;
	if (get_inode_from_iid(&superblock, inode_id, &inode) != 0)
		return -ENOENT;

	*st = inode->stats;
	return EXIT_SUCCESS;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir(%s)\n", path);
	// get directory inode
	int inode_id = get_iid_from_path(&superblock, path);
	inode_t *directory;
	int ret_value = get_inode_from_iid(&superblock, inode_id, &directory);
	if (ret_value != EXIT_SUCCESS)
		return ret_value;

	// get dir_entry inode
	inode_t *dir_entry_inode;
	dir_entry_t dir_entry;
	read_directory(directory, offset, &dir_entry);
	offset += sizeof(dir_entry_t);
	while (dir_entry.inode_id >= 0) {  // dir_entry is valid
		assert(get_inode_from_iid(&superblock,
		                          dir_entry.inode_id,
		                          &dir_entry_inode) ==
		       EXIT_SUCCESS);  // get dir_entry's inode
		if (filler(buffer, dir_entry.name, &dir_entry_inode->stats, offset)) {
			printf("[debug] fisopfs_readdir: filler returned "
			       "something other than 0\n");
			return 1;  // ERROR, filler's buffer is full
		}

		read_directory(directory, offset, &dir_entry);
		offset += sizeof(dir_entry_t);
	}

	return 0;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir(%s)\n", path);
	char parent_dir_path[PATH_MAX];
	char dir_name[MAX_FILENAME_LENGTH];
	char *last_slash_pos = strrchr(path, '/');
	// strncpy so that parent_dir_path contains the trailling slash
	memcpy(parent_dir_path, path, last_slash_pos - path + 1);
	parent_dir_path[last_slash_pos - path + 2] = '\0';
	// new_dir's name does not contain slashes
	strcpy(dir_name, last_slash_pos + 1);
	int inode_id = get_iid_from_path(&superblock, parent_dir_path);
	if (inode_id < 0) {
		return -1;
	}
	return create_dir(&superblock, dir_name, inode_id) <
	       0;  // returns 0 if create_dir succeeds
}

#define MAX_CONTENIDO
static char fisop_file_contenidos[MAX_CONTENIDO] = "hola fisopfs!\n";

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read(%s, %lu, %lu)\n", path, offset, size);

	// Solo tenemos un archivo hardcodeado!
	if (strcmp(path, "/fisop") != 0)
		return -ENOENT;

	if (offset + size > strlen(fisop_file_contenidos))
		size = strlen(fisop_file_contenidos) - offset;

	size = size > 0 ? size : 0;

	strncpy(buffer, fisop_file_contenidos + offset, size);

	return size;
}

static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.mkdir = fisopfs_mkdir,
	.read = fisopfs_read,
};

int
main(int argc, char *argv[])
{
	// init filesystem
	bitmap_set_all_1(
	        &superblock.free_tables_bitmap);  // mark all tables as free/unused
	// initialise root_dir
	create_dir(&superblock, "/", ROOT_DIR_INODE_ID);
	return fuse_main(argc, argv, &operations, NULL);
}

// fuse operations que probablemente haya que implementar:
// struct fuse_operations {

/** Remove a file */
//	int (*unlink) (const char *);

/** Remove a directory */
//	int (*rmdir) (const char *);

/** Rename a file */
//	int (*rename) (const char *, const char *);

/** Change the size of a file */
//	int (*truncate) (const char *, off_t);

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.	 An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
//	int (*read) (const char *, char *, size_t, off_t,
//		     struct fuse_file_info *);

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.	 An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
//	int (*write) (const char *, const char *, size_t, off_t,
//		      struct fuse_file_info *);

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
//	int (*create) (const char *, mode_t, struct fuse_file_info *);

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
//	int (*ftruncate) (const char *, off_t, struct fuse_file_info *);

//};
