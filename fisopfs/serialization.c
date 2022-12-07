#define _GNU_SOURCE
#include "serialization.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

int
write_filesystem_data(superblock_t *superblock, int data_fd)
{
	bitmap128_t *bitmap = &superblock->free_tables_bitmap;

	if (write(data_fd, bitmap, sizeof(bitmap128_t)) != sizeof(bitmap128_t)) {
		return EXIT_FAILURE;
	}

	for (int i = 0; i < AMOUNT_OF_INODE_TABLES; i++) {
		if (bitmap_getbit(bitmap, i))
			continue;

		inode_table_t *table = superblock->inode_tables[i];
		if (write(data_fd, table, PAGE_SIZE) != PAGE_SIZE)
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int
write_inode_pages(inode_t *inode, int data_fd)
{
	for (int k = 0; k < inode->stats.st_blocks; k++) {
		char *page = inode->pages[k];
		if (write(data_fd, page, PAGE_SIZE) != PAGE_SIZE)
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int
write_user_data(superblock_t *superblock, int data_fd)
{
	bitmap128_t *bitmap = &superblock->free_tables_bitmap;

	for (int i = 0; i < AMOUNT_OF_INODE_TABLES; i++) {
		if (bitmap_getbit(bitmap, i))
			continue;

		inode_table_t *table = superblock->inode_tables[i];

		bitmap128_t *bitmap = &table->free_inodes_bitmap;
		inode_t *inodes = table->inodes;

		for (int j = 0; j < INODES_PER_TABLE; j++) {
			if (bitmap_getbit(bitmap, j))
				continue;

			inode_t *inode = inodes + j;
			if (write_inode_pages(inode, data_fd) == EXIT_FAILURE)
				return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int
read_filesystem_data(superblock_t *superblock, int data_fd)
{
	bitmap128_t *bitmap = &superblock->free_tables_bitmap;

	if (read(data_fd, bitmap, sizeof(bitmap128_t)) != sizeof(bitmap128_t)) {
		return EXIT_FAILURE;
	};

	for (int i = 0; i < AMOUNT_OF_INODE_TABLES; i++) {
		if (bitmap_getbit(bitmap, i))
			continue;

		inode_table_t *inode_table = mmap(NULL,
		                                  PAGE_SIZE,
		                                  PROT_READ | PROT_WRITE,
		                                  MAP_ANONYMOUS | MAP_PRIVATE,
		                                  -1,
		                                  0);
		if (read(data_fd, inode_table, PAGE_SIZE) != PAGE_SIZE)
			return EXIT_FAILURE;

		superblock->inode_tables[i] = inode_table;
	}
	return EXIT_SUCCESS;
}

int
read_inode_pages(inode_t *inode, int data_fd)
{
	for (int k = 0; k < inode->stats.st_blocks; k++) {
		char *page = mmap(NULL,
		                  PAGE_SIZE,
		                  PROT_READ | PROT_WRITE,
		                  MAP_ANONYMOUS | MAP_PRIVATE,
		                  -1,
		                  0);
		if (read(data_fd, page, PAGE_SIZE) != PAGE_SIZE)
			return EXIT_FAILURE;
		inode->pages[k] = page;
	}
	return EXIT_SUCCESS;
}

int
read_user_data(superblock_t *superblock, int data_fd)
{
	bitmap128_t *bitmap = &superblock->free_tables_bitmap;

	for (int i = 0; i < AMOUNT_OF_INODE_TABLES; i++) {
		if (bitmap_getbit(bitmap, i))
			continue;

		inode_table_t *table = superblock->inode_tables[i];
		bitmap128_t *table_bitmap = &(table->free_inodes_bitmap);
		inode_t *table_inodes = table->inodes;

		for (int j = 0; j < INODES_PER_TABLE; j++) {
			if (bitmap_getbit(table_bitmap, j))
				continue;

			inode_t *inode = table_inodes + j;

			if (read_inode_pages(inode, data_fd) != EXIT_SUCCESS) {
				return EXIT_FAILURE;
			}
		}
	}
	return EXIT_SUCCESS;
}

int
serialize(superblock_t *superblock, const char *path)
{
	int data_fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
	if (data_fd == -1)
		return EXIT_FAILURE;

	if (write_filesystem_data(superblock, data_fd) == EXIT_FAILURE)
		return EXIT_FAILURE;
	if (write_user_data(superblock, data_fd) == EXIT_FAILURE)
		return EXIT_FAILURE;
	close(data_fd);
	return EXIT_SUCCESS;
}

int
deserialize(superblock_t *superblock, const char *path)
{
	int data_fd = open(path, O_RDONLY);
	if (data_fd == -1) {
		return EXIT_FAILURE;
	}

	if (read_filesystem_data(superblock, data_fd) != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}
	if (read_user_data(superblock, data_fd) != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	close(data_fd);
	return EXIT_SUCCESS;
}
