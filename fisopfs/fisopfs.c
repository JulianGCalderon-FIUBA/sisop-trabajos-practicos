#define FUSE_USE_VERSION 30

#define _GNU_SOURCE
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
#include <sys/mman.h>

superblock_t superblock;
#define ALL_PERMISSIONS (S_IRWXU | S_IRWXG | S_IRWXO)

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr(%s)\n", path);

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
	printf("[debug] fisopfs_create(%s, 0%o)\n", path, mode);

	if (get_iid_from_path(&superblock, path) >= 0)
		return EEXIST;  // file exists

	char parent_dir_path[PATH_MAX];
	char *file_name = split_path(path, parent_dir_path);
	int dir_id = get_iid_from_path(&superblock, parent_dir_path);
	int ret_val =
	        create_file(&superblock, file_name, dir_id, mode, file_info);
	return ret_val < 0 ? -ret_val : EXIT_SUCCESS;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read(%s, %lu, %lu)\n", path, offset, size);

	inode_t *file_inode;
	if (get_inode_from_path(&superblock, path, &file_inode) != EXIT_SUCCESS) {
		return ENOENT;
	}

	ssize_t size_read = inode_read(buffer, size, file_inode, offset);
	if (size_read < size)
		memset(&buffer[size_read], 0, size - size_read);

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

	ssize_t size_written =
	        inode_write((char *) buffer, size, file_inode, offset);

	// return either bytes written, or error stored in size_wrote
	return size_written;
}


static int
fisopfs_truncate(const char *path, off_t offset)
{
	printf("[debug] fisopfs_truncate(%s, %lu)\n", path, offset);
	inode_t *inode;
	if (get_inode_from_path(&superblock, path, &inode) != EXIT_SUCCESS)
		return ENOENT;
	return inode_truncate(inode, offset);
}

static int
fisopfs_rename(const char *old_path, const char *new_path)
{
	char old_parent_dir_path[PATH_MAX];
	char *old_name = split_path(old_path, old_parent_dir_path);

	char new_parent_dir_path[PATH_MAX];
	char *new_name = split_path(new_path, new_parent_dir_path);

	int ret_val = 0;
	inode_t *old_dir_inode;
	inode_t *new_dir_inode;
	inode_t *file_inode;
	ret_val |= get_inode_from_path(&superblock,
	                               old_parent_dir_path,
	                               &old_dir_inode);
	ret_val |= get_inode_from_path(&superblock,
	                               new_parent_dir_path,
	                               &new_dir_inode);
	ret_val |= get_inode_from_path(&superblock, old_path, &file_inode);
	if (ret_val != EXIT_SUCCESS)
		return ENOENT;

	ret_val = create_dir_entry(
	        &superblock, new_dir_inode, file_inode->stats.st_ino, new_name);
	if (ret_val != EXIT_SUCCESS)
		return ret_val;

	ret_val = unlink_dir_entry(&superblock, old_dir_inode, old_name);
	return ret_val;
}


void
fisopfs_destroy(void *private_data)
{
	int data_fd = open("data.fisopfs", O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
	if (data_fd == -1)
		return;

	bitmap128_t *bitmap = &superblock.free_tables_bitmap;

	if (write(data_fd, bitmap, sizeof(bitmap128_t)) != sizeof(bitmap128_t))
		return;

	printf("wrote bitmap\n");

	printf("for each occupied inode table\n");

	for (int i = 0; i < AMOUNT_OF_INODE_TABLES; i++) {
		if (bitmap_getbit(bitmap, i))
			continue;

		inode_table_t *table = superblock.inode_tables[i];
		if (write(data_fd, table, PAGE_SIZE) != PAGE_SIZE)
			return;

		printf("-wrote inode table %i\n", i);
	}

	printf("wrote all inode tables\n");


	for (int i = 0; i < AMOUNT_OF_INODE_TABLES; i++) {
		if (bitmap_getbit(bitmap, i))
			continue;

		printf("-accessing inode table %i\n", i);


		inode_table_t *table = superblock.inode_tables[i];

		bitmap128_t *bitmap = &table->free_inodes_bitmap;
		inode_t *inodes = table->inodes;

		printf("-for every inode in table...\n");

		for (int j = 0; j < INODES_PER_TABLE; j++) {
			if (bitmap_getbit(bitmap, j))
				continue;

			printf("--accesing inode %i\n", j);

			inode_t *inode = inodes + j;

			for (int k = 0; k < inode->stats.st_blocks; k++) {
				char *page = inode->pages[k];
				if (write(data_fd, page, PAGE_SIZE) != PAGE_SIZE)
					return;

				printf("--wrote page %i\n", k);
			}
		}
	}

	close(data_fd);
}


static struct fuse_operations operations = {
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.mkdir = fisopfs_mkdir,
	.rmdir = fisopfs_rmdir,
	.unlink = fisopfs_unlink,
	.create = fisopfs_create,
	.read = fisopfs_read,
	.write = fisopfs_write,
	.truncate = fisopfs_truncate,
	.rename = fisopfs_rename,
	.destroy = fisopfs_destroy,
};

int
deserialize(char *path)
{
	int data_fd = open("data.fisopfs", O_RDONLY);
	if (data_fd == -1) {
		return -1;
	}

	bitmap128_t *bitmap = &superblock.free_tables_bitmap;

	if (read(data_fd, bitmap, sizeof(bitmap128_t)) != sizeof(bitmap128_t)) {
		return -1;
	};


	for (int i = 0; i < AMOUNT_OF_INODE_TABLES; i++) {
		if (bitmap_getbit(bitmap, i))
			continue;


		inode_table_t *inode_table = mmap(NULL,
		                                  PAGE_SIZE,
		                                  PROT_READ | PROT_WRITE,
		                                  MAP_ANON | MAP_PRIVATE,
		                                  -1,
		                                  0);
		if (read(data_fd, inode_table, PAGE_SIZE) != PAGE_SIZE)
			return -1;

		superblock.inode_tables[i] = inode_table;
	}


	for (int i = 0; i < AMOUNT_OF_INODE_TABLES; i++) {
		if (bitmap_getbit(bitmap, i))
			continue;


		inode_table_t *table = superblock.inode_tables[i];
		bitmap128_t *table_bitmap = &(table->free_inodes_bitmap);
		inode_t *table_inodes = table->inodes;


		for (int j = 0; j < INODES_PER_TABLE; j++) {
			if (bitmap_getbit(table_bitmap, j))
				continue;


			inode_t *inode = table_inodes + j;

			for (int k = 0; k < inode->stats.st_blocks; k++) {
				char *page = mmap(NULL,
				                  PAGE_SIZE,
				                  PROT_READ | PROT_WRITE,
				                  MAP_ANON | MAP_PRIVATE,
				                  -1,
				                  0);
				if (read(data_fd, page, PAGE_SIZE) != PAGE_SIZE)
					return -1;
				inode->pages[k] = page;
			}
		}
	}


	close(data_fd);

	return 0;
}


int
main(int argc, char *argv[])
{
	if (deserialize("data.fisopfs")) {
		// mark all tables as free/unused
		bitmap_set_all_1(&superblock.free_tables_bitmap);

		// initialise root_dir
		create_dir(&superblock, "/", ROOT_DIR_INODE_ID, ALL_PERMISSIONS);
	}

	return fuse_main(argc, argv, &operations, NULL);
}

// fuse operations que probablemente haya que implementar:
// struct fuse_operations {


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
