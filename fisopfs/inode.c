#define _GNU_SOURCE
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <linux/limits.h>
#include "bitmap.h"
#include "inode.h"
#include "dir.h"
#include <stdio.h>


#define ALL_PERMISSIONS (S_IRWXU | S_IRWXG | S_IRWXO)

/*
 *
 */
int
get_inode_from_iid(superblock_t *superblock, int inode_id, inode_t **inode_dest)
{
	if (inode_id < 0)
		return EINVAL;

	int table_num = inode_id / INODES_PER_TABLE;

	// if table is free, there is no inode
	if (bitmap_getbit(&superblock->free_tables_bitmap,
	                  table_num)) {  // table is free and therefore empty
		*inode_dest = NULL;
		return ENOENT;
	}

	int inode_pos = inode_id % INODES_PER_TABLE;

	// if inode is free,
	if (bitmap_getbit(&superblock->inode_tables[table_num]->free_inodes_bitmap,
	                  inode_pos)) {
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
int
malloc_inode_table(superblock_t *superblock)
{
	int free_table_num =
	        bitmap_count_leading_zeros(&superblock->free_tables_bitmap);
	if (free_table_num < 0)  // The array superblock->inode_tables is full
		return -ENOMEM;

	inode_table_t *inode_table = mmap(NULL,
	                                  PAGE_SIZE,
	                                  PROT_READ | PROT_WRITE,
	                                  MAP_ANON | MAP_PRIVATE,
	                                  -1,
	                                  0);
	if (inode_table == NULL)
		return -ENOMEM;

	superblock->inode_tables[free_table_num] = inode_table;

	// sets table bit to occupied
	bitmap_clearbit(&superblock->free_tables_bitmap, free_table_num);

	// sets all inode bits to occupied, as there are some unusable bit positions
	bitmap_set_all_0(&inode_table->free_inodes_bitmap);

	// sets usable inode bits to free
	for (int inode_pos = 0; inode_pos < INODES_PER_TABLE; ++inode_pos)
		bitmap_setbit(&inode_table->free_inodes_bitmap, inode_pos);

	return free_table_num;
}

/*
 *
 */
int
free_inode_table(superblock_t *superblock, int table_num)
{
	bitmap_setbit(&superblock->free_tables_bitmap, table_num);
	return munmap(superblock->inode_tables[table_num], PAGE_SIZE);
}

int
malloc_inode_page(inode_t *inode, int page_num)
{
	if (page_num > PAGES_PER_INODE)
		return EFBIG;

	char *page = mmap(NULL,
	                  PAGE_SIZE,
	                  PROT_READ | PROT_WRITE,
	                  MAP_ANON | MAP_PRIVATE,
	                  -1,
	                  0);
	if (page == NULL)
		return ENOMEM;

	inode->pages[page_num] = page;
	inode->stats.st_blocks++;

	return EXIT_SUCCESS;
}

int
free_inode_page(inode_t *inode, int page_num)
{
	char *page = inode->pages[page_num];
	if (!page)
		return EXIT_SUCCESS;

	inode->pages[page_num] = NULL;
	--inode->stats.st_blocks;

	return munmap(page, PAGE_SIZE);
}

void
init_inode(inode_t *inode, int inode_id)
{
	struct stat *inode_st = &inode->stats;

	inode_st->st_nlink = 1;
	// podríamos usar umask para ver los default
	inode_st->st_mode = ALL_PERMISSIONS;
	inode_st->st_blocks = 0;
	inode_st->st_ino = inode_id;
	inode_st->st_size = 0;

	// CORREGIR: deberíamos setearlo al usuario actual (getcuruid? algo así)
	inode_st->st_uid = 1000;
	// CORREGIR: deberíamos setearlo al grupo actual
	inode_st->st_gid = 1000;

	time(&inode_st->st_mtime);
	time(&inode_st->st_ctime);
	time(&inode_st->st_atime);

	// sets all pages to NULL, as it initates with no alloced pages
	for (int page_num = 0; page_num < PAGES_PER_INODE; ++page_num)
		inode->pages[page_num] = NULL;
}


/* Returns the id of the first free inode in the superblock. If no free inode
 * was found, it returns -1
 */
int
first_free_inode(superblock_t *superblock)
{
	inode_table_t *table;
	bitmap128_t *free_tables_bmap = &superblock->free_tables_bitmap;

	for (int table_num = 0; table_num < AMOUNT_OF_INODE_TABLES; table_num++) {
		// if table doesn't exist, it has not been allocated
		if (bitmap_getbit(free_tables_bmap, table_num))
			continue;

		table = superblock->inode_tables[table_num];

		// if table is full, does not contains free inodes
		if (!bitmap_has_set_bit(&table->free_inodes_bitmap))
			continue;

		// first free inode in table
		int free_inode_id =
		        table_num * INODES_PER_TABLE +
		        bitmap_count_leading_zeros(&table->free_inodes_bitmap);

		return free_inode_id;
	}

	return -1;
}

/*
 * Marks a free inode as occupied, and returns a pointer to the inode.
 * The inode stats are not initialised, except for the inode_id.
 * Does not request memory. If there are no free inodes, returns NULL.
 */
inode_t *
alloc_inode(superblock_t *superblock)
{
	int free_inode_id = first_free_inode(superblock);
	if (free_inode_id == -1) {
		return NULL;
	}

	int table_num = free_inode_id / INODES_PER_TABLE;
	int inode_id_in_table = free_inode_id % INODES_PER_TABLE;

	inode_table_t *table = superblock->inode_tables[table_num];

	// set inode as occupied
	bitmap_clearbit(&table->free_inodes_bitmap, inode_id_in_table);

	inode_t *free_inode = &table->inodes[inode_id_in_table];
	init_inode(free_inode, free_inode_id);

	return free_inode;
}


/*
 * Marks a free inode as occupied, and returns a pointer to the inode.
 * Upon failure, returns NULL.
 * The inode stats are not initialised, except for the inode_id.
 * Internally requests memory with mmap if necessary, which is later
 * freed by destroy_filesystem
 */
inode_t *
malloc_inode(superblock_t *superblock)
{
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
	inode_dest = &table->inodes[0];

	// there are no alloc'd inodes in the table, so first position is free
	bitmap_clearbit(&table->free_inodes_bitmap, 0);
	init_inode(inode_dest, table_num * INODES_PER_TABLE);
	return inode_dest;
}

/*
 * Marks an inode as free so it can be reused.
 * Frees a memory page if there are no alloc'd inodes in the page.
 */
void
free_inode(superblock_t *superblock, int inode_id)
{
	// free inode internal pages
	inode_t *inode;
	get_inode_from_iid(superblock, inode_id, &inode);
	for (int page_num = 0; page_num < PAGES_PER_INODE; ++page_num)
		free_inode_page(inode, page_num);

	// mark inode as free
	int table_num = inode_id / INODES_PER_TABLE;
	int inode_pos = inode_id % INODES_PER_TABLE;
	bitmap_setbit(&superblock->inode_tables[table_num]->free_inodes_bitmap,
	              inode_pos);

	// if it is the last inode in a table, free the table
	if (!bitmap_has_unset_bit(
	            &superblock->inode_tables[table_num]->free_inodes_bitmap))
		free_inode_table(superblock, table_num);
}

size_t
min(size_t x, size_t y)
{
	return x < y ? x : y;
}

size_t
max(size_t x, size_t y)
{
	return x > y ? x : y;
}

ssize_t
inode_read(char *buffer, size_t total_bytes_to_read, inode_t *inode, size_t file_offset)
{
	if (total_bytes_to_read == 0)
		return 0;
	if (file_offset > inode->stats.st_size)
		return -EINVAL;

	int cur_page_num = file_offset / PAGE_SIZE;
	size_t page_offset = file_offset % PAGE_SIZE;

	size_t file_remaining_bytes = inode->stats.st_size - file_offset;
	total_bytes_to_read = min(total_bytes_to_read, file_remaining_bytes);

	size_t bytes_remaining = total_bytes_to_read;
	size_t bytes_to_read;
	char *page;
	// read one page at a time, and return amount of bytes read
	while (bytes_remaining > 0) {
		page = inode->pages[cur_page_num];
		size_t page_remaining_bytes = PAGE_SIZE - page_offset;
		bytes_to_read = min(bytes_remaining, page_remaining_bytes);
		memcpy(buffer, page + page_offset, bytes_to_read);

		buffer += bytes_to_read;
		bytes_remaining -= bytes_to_read;
		cur_page_num++;
		page_offset = 0;
	}

	return total_bytes_to_read - bytes_remaining;
}

void
free_unneeded_pages(inode_t *inode, int pages_to_free)
{
	int number_of_pages = inode->stats.st_blocks;

	for (int i = 0; i < pages_to_free; i++) {
		free_inode_page(inode, number_of_pages - 1 - i);
	}
}

ssize_t
alloc_needed_pages(inode_t *inode, int pages_to_alloc)
{
	int initial_number_of_pages = inode->stats.st_blocks;

	for (int i = 0; i < pages_to_alloc; i++) {
		int ret_val =
		        malloc_inode_page(inode, initial_number_of_pages + i);

		if (ret_val != EXIT_SUCCESS) {
			free_unneeded_pages(inode, i);
			return -ret_val;
		}
	}
	return EXIT_SUCCESS;
}


ssize_t
inode_write(char *buffer, size_t buffer_len, inode_t *inode, size_t file_offset)
{
	if (buffer_len == 0)
		return 0;

	// if an attempt to write exceeds maximum file size
	size_t final_offset = file_offset + buffer_len;
	int needed_pages = ((final_offset - 1) / PAGE_SIZE) + 1;
	if (needed_pages > PAGES_PER_INODE) {
		return -EFBIG;
	}


	// if offset is bigger than size
	if (file_offset > inode->stats.st_size)
		return -EINVAL;

	// allocs memory for all needed pages
	int pages_to_alloc = needed_pages - inode->stats.st_blocks;


	int ret_val = alloc_needed_pages(inode, pages_to_alloc);
	if (ret_val != EXIT_SUCCESS)
		return -ret_val;

	int cur_page_num = file_offset / PAGE_SIZE;
	size_t bytes_to_write;
	size_t bytes_remaining = buffer_len;
	size_t page_offset = file_offset % PAGE_SIZE;
	char *page;
	// write one page at a time, and return amount of bytes written
	while (bytes_remaining > 0) {
		page = inode->pages[cur_page_num];

		size_t page_remaining_bytes = PAGE_SIZE - page_offset;
		bytes_to_write = min(bytes_remaining, page_remaining_bytes);
		memcpy(page + page_offset, buffer, bytes_to_write);

		bytes_remaining -= bytes_to_write;
		cur_page_num++;
		page_offset = 0;
	}

	inode->stats.st_size = max(inode->stats.st_size, final_offset);
	return buffer_len;
}

int
highest_page_number_for_size(off_t size)
{
	return size ? (size - 1) / PAGES_PER_INODE : 0;
}


/*
 * Inode changes size to min(inode.stats.st_size, offset)
 * Bytes at the end of the file are removed
 */
void
inode_truncate(inode_t *inode, size_t new_size)
{
	int size_difference = new_size - inode->stats.st_size;
	if (size_difference > 0) {
		//  todo!
		return;
	}


	int highest_page_num = highest_page_number_for_size(inode->stats.st_size);
	int new_highest_page_num = highest_page_number_for_size(new_size);

	int page_difference = new_highest_page_num - highest_page_num;
	free_unneeded_pages(inode, page_difference);

	inode->stats.st_size = new_size;
}

int
get_inode_from_path(superblock_t *superblock, const char *path, inode_t **inode_dest)
{
	int inode_id = get_iid_from_path(superblock, path);
	return get_inode_from_iid(superblock, inode_id, inode_dest);
}
