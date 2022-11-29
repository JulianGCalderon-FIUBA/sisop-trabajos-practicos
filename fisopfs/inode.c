#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <linux/limits.h>
#include "bitmap.h"
#include "inode.h"

/*
 * 
 */
int get_inode_from_iid(superblock_t *superblock, int inode_id, inode_t **inode_dest) {
	if (inode_id < 0)
		return EINVAL;

	int table_num = inode_id / INODES_PER_TABLE;
	if (bitmap_getbit(&superblock->free_tables_bitmap, table_num)) { // table is free and therefore empty 
		*inode_dest = NULL;
		return ENOENT;
	}
	int inode_pos = inode_id % INODES_PER_TABLE;
	if (bitmap_getbit(&superblock->inode_tables[table_num]->free_inodes_bitmap, inode_pos)) { // inode is free
		*inode_dest = NULL;
		return ENOENT;
	}
	*inode_dest = &superblock->inode_tables[table_num]->inodes[inode_pos];
	return EXIT_SUCCESS;
}

/*
 * Returns the position of the table in superblock->inode_tables,
 * or a negative error code upon failure.
 */
int malloc_inode_table(superblock_t *superblock) {
	int free_table_num = bitmap_count_leading_zeros(&superblock->free_tables_bitmap);
	if (free_table_num < 0) // The array superblock->inode_tables is full
		return -ENOMEM;

	inode_table_t *inode_table = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (inode_table == NULL)
		return -ENOMEM;
	bitmap_clearbit(&superblock->free_tables_bitmap, free_table_num);
	superblock->inode_tables[free_table_num] = inode_table;
	bitmap_set_all_0(&inode_table->free_inodes_bitmap);
	for (int inode_pos = 0; inode_pos < INODES_PER_TABLE; ++inode_pos)
		bitmap_setbit(&inode_table->free_inodes_bitmap, inode_pos);
	return free_table_num;
}

/*
 * 
 */
int free_inode_table(superblock_t *superblock, int table_num) {
	bitmap_setbit(&superblock->free_tables_bitmap, table_num);
	return munmap(superblock->inode_tables[table_num], PAGE_SIZE);
}

/*
 * Marks a free inode as occupied, and returns a pointer to the inode.
 * The inode stats are not initialised, except for the inode_id.
 * Does not request memory. If there are no free inodes, returns NULL.
 */
inode_t *alloc_inode(superblock_t *superblock) {
	inode_t *inode_dest = NULL;
	int is_table_free;
	int free_inode_id;
	inode_table_t *table;
	bitmap128_t *free_tables_bmap = &superblock->free_tables_bitmap;
	for (int table_num = 0; table_num < (sizeof(bitmap128_t) * 8); ++table_num) {
		is_table_free = bitmap_getbit(free_tables_bmap, table_num);
		if (is_table_free) // table doesn't exist, it has not been allocated
			continue;
		table = superblock->inode_tables[table_num];
		if (!bitmap_has_set_bit(&table->free_inodes_bitmap)) // there are no free inodes in the table
			continue;
		free_inode_id = bitmap_count_leading_zeros(&table->free_inodes_bitmap); // first free inode in table
		bitmap_clearbit(&table->free_inodes_bitmap, free_inode_id); // set inode as occupied
		inode_dest = &table->inodes[free_inode_id];
		inode_dest->stats.st_ino = table_num * INODES_PER_TABLE + free_inode_id;
		break;
	}
	if (!inode_dest)
		return NULL;
	for (int page_num = 0; page_num < PAGES_PER_INODE; ++page_num)
		inode_dest->pages[page_num] = NULL;
	inode_dest->stats.st_size = 0;
	return inode_dest;
}

/*
 * Marks a free inode as occupied, and returns a pointer to the inode.
 * Upon failure, returns NULL.
 * The inode stats are not initialised, except for the inode_id.
 * Internally requests memory with mmap if necessary, which is later freed by destroy_filesystem
 */
inode_t *malloc_inode(superblock_t *superblock) {
	inode_table_t *table;
	inode_t *inode_dest;
	if ((inode_dest = alloc_inode(superblock)))
		return inode_dest;

	// no free inodes, request memory
	int table_num = malloc_inode_table(superblock);
	if (table_num < 0) {
		return NULL;
	}
	table = superblock->inode_tables[table_num];
	bitmap_clearbit(&table->free_inodes_bitmap, 0); // set first inode as occupied
	inode_dest = &table->inodes[0];
	inode_dest->stats.st_ino = table_num * INODES_PER_TABLE;
	for (int page_num = 0; page_num < PAGES_PER_INODE; ++page_num)
		inode_dest->pages[page_num] = NULL;
	inode_dest->stats.st_size = 0;
	return inode_dest;
}

/*
 * Marks an inode as free so it can be reused.
 * Frees a memory page if there are no alloc'd inodes in the page.
 */
void free_inode(superblock_t *superblock, int inode_id) {
	int table_num = inode_id / INODES_PER_TABLE;
	int inode_pos = inode_id % INODES_PER_TABLE;
	bitmap_setbit(&superblock->inode_tables[table_num]->free_inodes_bitmap, inode_pos);
	if (bitmap_has_unset_bit(&superblock->inode_tables[table_num]->free_inodes_bitmap))
		return;

	// there are no alloc'd inodes in the table
	free_inode_table(superblock, table_num);
}

int malloc_inode_page(inode_t *inode, int page_num) {
	if (page_num > PAGES_PER_INODE)
		return EFBIG;
	char *page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (page == NULL)
		return ENOMEM;
	inode->pages[page_num] = page;
	return EXIT_SUCCESS;
}

size_t min(size_t x, size_t y) {
	return x < y ? x : y;
}

ssize_t inode_write(char *buffer, size_t buffer_len, inode_t *inode, size_t file_offset) {
	if (buffer_len == 0)
		return 0;
	int cur_page_num = file_offset / PAGE_SIZE;
	
	// if no bytes have been written, return ERROR CODE upon error
	if (inode->stats.st_size >= PAGE_SIZE * PAGES_PER_INODE)
		return -EFBIG; // "File too large" not very specific but better than ENOMEM
	if (file_offset > inode->stats.st_size)
		return -EINVAL;
	int ret_val;
	if (inode->pages[cur_page_num] == NULL) { // may happen if prev page is full, or this is the first page
		if ((ret_val = malloc_inode_page(inode, cur_page_num)) != EXIT_SUCCESS)
			return -ret_val;
	}

	// write one page at a time, and return amount of bytes written
	size_t bytes_to_write;
	size_t bytes_remaining = buffer_len;
	size_t page_offset = file_offset % PAGE_SIZE;
	char *page = inode->pages[cur_page_num];
	while (bytes_remaining > 0) {
		if (page == NULL) { // may happen if prev page is full, or this is the first page
			if (malloc_inode_page(inode, cur_page_num) != EXIT_SUCCESS)
				break;
			page = inode->pages[cur_page_num];
		}
		bytes_to_write = min(bytes_remaining, PAGE_SIZE - page_offset);
		memcpy(page + page_offset, buffer, bytes_to_write);
		bytes_remaining -= bytes_to_write;
		++cur_page_num;
		page_offset = 0;
		if (cur_page_num >= PAGES_PER_INODE)
			break;
		page = inode->pages[cur_page_num];
	}
	file_offset += buffer_len - bytes_remaining; // initial offset + bytes written
	inode->stats.st_size = inode->stats.st_size > file_offset ? inode->stats.st_size : file_offset;
	return buffer_len - bytes_remaining;
}

ssize_t inode_read(char *buffer, size_t ttl_bytes_to_read, inode_t *inode, size_t file_offset) {
	if (ttl_bytes_to_read == 0)
		return 0;
	if (file_offset > inode->stats.st_size)
		return -EINVAL;
	int cur_page_num = file_offset / PAGE_SIZE;
	size_t page_offset = file_offset % PAGE_SIZE;
	ttl_bytes_to_read = min(ttl_bytes_to_read, inode->stats.st_size - file_offset);
	size_t bytes_remaining = ttl_bytes_to_read;
	size_t bytes_to_read;
	char *page;
	// read one page at a time, and return amount of bytes read
	while (bytes_remaining > 0) {
		page = inode->pages[cur_page_num];
		bytes_to_read = min(bytes_remaining, PAGE_SIZE - page_offset);
		memcpy(buffer, page + page_offset, bytes_to_read);
		bytes_remaining -= bytes_to_read;
		++cur_page_num;
		page_offset = 0;
		page = NULL;
	}
	return ttl_bytes_to_read - bytes_remaining;
}
