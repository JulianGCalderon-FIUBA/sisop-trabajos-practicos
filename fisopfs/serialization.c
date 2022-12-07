#define _GNU_SOURCE
#include "serialization.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

void
serialize(superblock_t *superblock, const char *path)
{
	int data_fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
	if (data_fd == -1)
		return;

	bitmap128_t *bitmap = &superblock->free_tables_bitmap;

	if (write(data_fd, bitmap, sizeof(bitmap128_t)) != sizeof(bitmap128_t))
		return;


	for (int i = 0; i < AMOUNT_OF_INODE_TABLES; i++) {
		if (bitmap_getbit(bitmap, i))
			continue;

		inode_table_t *table = superblock->inode_tables[i];
		if (write(data_fd, table, PAGE_SIZE) != PAGE_SIZE)
			return;
	}


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

			for (int k = 0; k < inode->stats.st_blocks; k++) {
				char *page = inode->pages[k];
				if (write(data_fd, page, PAGE_SIZE) != PAGE_SIZE)
					return;
			}
		}
	}

	close(data_fd);
}


int
deserialize(superblock_t *superblock, const char *path)
{
	int data_fd = open(path, O_RDONLY);
	if (data_fd == -1) {
		return -1;
	}

	bitmap128_t *bitmap = &superblock->free_tables_bitmap;

	if (read(data_fd, bitmap, sizeof(bitmap128_t)) != sizeof(bitmap128_t)) {
		return -1;
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
			return -1;

		superblock->inode_tables[i] = inode_table;
	}


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

			for (int k = 0; k < inode->stats.st_blocks; k++) {
				char *page = mmap(NULL,
				                  PAGE_SIZE,
				                  PROT_READ | PROT_WRITE,
				                  MAP_ANONYMOUS | MAP_PRIVATE,
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
