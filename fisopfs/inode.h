#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <sys/stat.h>
#include "bitmap.h"

#define MAX_FILENAME_LENGTH 128
#define PAGES_PER_INODE 5 // amount of pages of memory that a file can point to. Max file size = PAGE_SIZE * PAGES_PER_INODE
#define PAGE_SIZE 4096 // must be a power of 2
#define AMOUNT_OF_INODE_TABLES 128 // DO NOT TOUCH unless you are also changing the bitmap logic

#define INODES_PER_TABLE ((PAGE_SIZE - sizeof(bitmap128_t)) / sizeof(inode_t)) // MAX 128, unless bitmap logic is also changed
#define DIR_ENTRIES_PER_PAGE (PAGE_SIZE / sizeof(dir_entry_t)) // must be at least 3

typedef struct {
	char name[MAX_FILENAME_LENGTH];
	int inode_id;
} dir_entry_t;

typedef struct {
	struct stat stats;
	time_t stat_crtime;
	char *pages[PAGES_PER_INODE];
} inode_t;

typedef struct {
	inode_t inodes[INODES_PER_TABLE];
	bitmap128_t free_inodes_bitmap;
} inode_table_t;

typedef struct {
	inode_table_t *inode_tables[AMOUNT_OF_INODE_TABLES];
	bitmap128_t free_tables_bitmap;
} superblock_t;


/*
 * Marks a free inode as occupied, and returns a pointer to the inode.
 * Upon failure, returns NULL.
 * The inode stats are not initialised, except for the inode_id.
 * Internally requests memory with mmap if necessary, which can be freed with free_inode.
 * Alternatively, calls to destroy_filesystem free all unfreed inodes.
 */
inode_t *malloc_inode(superblock_t *superblock);

/*
 * Marks an inode as free so it can be reused.
 * Frees a memory page if there are no alloc'd inodes in the page.
 */
void free_inode(superblock_t *superblock, int inode_id);

/*
 * Stores in inode_dest a pointer to the inode, or NULL in case of error.
 * The actual inode can be modified through the pointer, it is not a copy.
 */
int get_inode_from_iid(superblock_t *superblock, int inode_id, inode_t **inode_dest);

/*
 * 
 * 
 */
ssize_t inode_write(char *buffer, size_t buffer_len, inode_t *inode, size_t offset);

/*
 * DOES NOT Null terminate the buffer
 */
ssize_t inode_read(char *buffer, size_t bytes_to_read, inode_t *inode, size_t offset);

#endif // INODE_H