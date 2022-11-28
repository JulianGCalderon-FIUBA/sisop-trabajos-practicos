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

static int fisopfs_getattr(const char *path, struct stat *st) {
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

static int fisopfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
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
	while (dir_entry.inode_id >= 0) { // dir_entry is valid
		assert(get_inode_from_iid(&superblock, dir_entry.inode_id, &dir_entry_inode) == EXIT_SUCCESS); // get dir_entry's inode
		if ( filler(buffer, dir_entry.name, &dir_entry_inode->stats, offset) ) {
			printf("[debug] fisopfs_readdir: filler returned something other than 0\n");
			return 1; // ERROR, filler's buffer is full
		}

		read_directory(directory, offset, &dir_entry);
		offset += sizeof(dir_entry_t);
	}

	return 0;
}

#define MAX_CONTENIDO
static char fisop_file_contenidos[MAX_CONTENIDO] = "hola fisopfs!\n";

static int fisopfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
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
	.read = fisopfs_read,
};

int main(int argc, char *argv[]) {
	// init filesystem
	bitmap_set_all_1(&superblock.free_tables_bitmap); // mark all tables as free/unused
	// initialise root_dir
	assert(create_dir(&superblock, "/", ROOT_DIR_INODE_ID) == ROOT_DIR_INODE_ID);
	return fuse_main(argc, argv, &operations, NULL);
}
