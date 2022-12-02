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
#include "file.h"
#include "dir.h"

superblock_t superblock;
#define ALL_PERMISSIONS (S_IRWXU | S_IRWXG | S_IRWXO)

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr(%s)\n", path);

	// if ((inode_id = get_iid_from_path(&superblock, path)) < 0)
	// 	return -ENOENT;
	// if (get_inode_from_iid(&superblock, inode_id, &inode) != 0)
	// 	return -ENOENT;
	inode_t *inode;
	if (get_inode_from_path(&superblock, path, &inode) != 0) {
		return -ENOENT;
	}

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
	// int inode_id = get_iid_from_path(&superblock, path);
	// int ret_value = get_inode_from_iid(&superblock, inode_id,
	// &directory); if (ret_value != EXIT_SUCCESS) 	return ret_value;
	inode_t *directory;
	if (get_inode_from_path(&superblock, path, &directory)) {
		return -1;
	}

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
	char *new_dirs_name = split_path(path, parent_dir_path);
	int inode_id = get_iid_from_path(&superblock, parent_dir_path);
	if (inode_id < 0) {
		return -1;
	}
	return create_dir(&superblock, new_dirs_name, inode_id, mode) <
	       0;  // returns 0 if create_dir succeeds
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink(%s)\n", path);
	char parent_dir_path[PATH_MAX];
	char *childs_name = split_path(path, parent_dir_path);
	// int dir_id = get_iid_from_path(&superblock, parent_dir_path);
	// if (get_inode_from_iid(&superblock, dir_id, &dir) != EXIT_SUCCESS) {
	// 	return ENOENT;
	// }

	inode_t *dir;
	if (get_inode_from_path(&superblock, parent_dir_path, &dir) !=
	    EXIT_SUCCESS) {
		return ENOENT;
	}
	return unlink_dir_entry(&superblock, dir, childs_name);
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir(%s)\n", path);
	return fisopfs_unlink(path);
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *file_info)
{
	printf("[debug] fisopfs_create(%s, %d)\n", path, mode);

	if (get_iid_from_path(&superblock, path) >= 0)
		return EEXIST;  // file exists

	char parent_dir_path[PATH_MAX];
	char *file_name = split_path(path, parent_dir_path);
	int dir_id = get_iid_from_path(&superblock, parent_dir_path);
	int ret_val =
	        create_file(&superblock, file_name, dir_id, mode, file_info);
	return ret_val < 0 ? -ret_val : EXIT_SUCCESS;
}

#define MAX_CONTENIDO
// static char fisop_file_contenidos[MAX_CONTENIDO] = "hola fisopfs!\n";

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read(%s, %lu, %lu)\n", path, offset, size);

	// int inode_id = get_iid_from_path(&superblock, path);
	// if (get_inode_from_iid(&superblock, inode_id, &file_inode) !=
	// EXIT_SUCCESS) 	return ENOENT;

	inode_t *file_inode;
	if (get_inode_from_path(&superblock, path, &file_inode) != EXIT_SUCCESS) {
		return ENOENT;
	}

	ssize_t size_read = inode_read(buffer, size, file_inode, offset);
	if (size_read < size)
		memset(&buffer[size_read], 0, size - size_read);

	// fi->flags = file_inode->stats->flags;
	return size_read;
}


/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.	 An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
static int
fisopfs_write(const char *path,
              const char *buffer,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	inode_t *file_inode;
	if (get_inode_from_path(&superblock, path, &file_inode) != EXIT_SUCCESS) {
		return ENOENT;
	}

	ssize_t size_wrote =
	        inode_write((char *) buffer, size, file_inode, offset);

	if (size_wrote < size) {
		return -1;
	}

	return size_wrote;
}


static int
fisopfs_truncate(const char *path, off_t offset)
{
	printf("[debug] fisopfs_truncate(%s, %lu)\n", path, offset);
	inode_t *inode;
	int inode_id = get_iid_from_path(&superblock, path);
	if (get_inode_from_iid(&superblock, inode_id, &inode) != EXIT_SUCCESS)
		return ENOENT;
	inode_truncate(inode, offset);
	return EXIT_SUCCESS;
}

static struct fuse_operations operations = { .getattr = fisopfs_getattr,
	                                     .readdir = fisopfs_readdir,
	                                     .mkdir = fisopfs_mkdir,
	                                     .rmdir = fisopfs_rmdir,
	                                     .unlink = fisopfs_unlink,
	                                     .create = fisopfs_create,
	                                     .read = fisopfs_read,
	                                     .write = fisopfs_write,
	                                     .truncate = fisopfs_truncate };

int
main(int argc, char *argv[])
{
	// init filesystem
	bitmap_set_all_1(
	        &superblock.free_tables_bitmap);  // mark all tables as free/unused
	// initialise root_dir
	create_dir(&superblock, "/", ROOT_DIR_INODE_ID, ALL_PERMISSIONS);
	return fuse_main(argc, argv, &operations, NULL);
}

// fuse operations que probablemente haya que implementar:
// struct fuse_operations {

/** Rename a file */
//	int (*rename) (const char *, const char *);

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

/*  *
 * Change the access and modification times of a file with
 * nanosecond resolution
 *
 * This supersedes the old utime() interface.  New applications
 * should use this.
 *
 * See the utimensat(2) man page for details.
 *
 * Introduced in version 2.6
 */
//	int (*utimens) (const char *, const struct timespec tv[2]);
//};
