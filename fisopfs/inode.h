#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <sys/stat.h>
#include "bitmap.h"

#define MAX_FILENAME_LENGTH 128
#define PAGES_PER_INODE 5
#define PAGE_SIZE 4096 // must be a power of 2
#define INODES_PER_TABLE (PAGE_SIZE - sizeof(bitmap128_t)) / sizeof(inode_t)
#define DIR_ENTRIES_PER_PAGE PAGE_SIZE / sizeof(dir_entry_t) // must be at least 3
#define AMOUNT_OF_INODE_TABLES 128


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
 * Returns the dir_entry at offset within the dir directory.
 */
dir_entry_t *read_directory(inode_t *dir, size_t offset);

/*
 * Stores in inode_dest a pointer to the inode, or NULL in case of error.
 * The actual inode can be modified through the pointer, it is not a copy.
 */
int get_inode_by_id(superblock_t *superblock, int inode_id, inode_t **inode_dest);

/*
 * 
 */
int get_inode_id(superblock_t *superblock, const char *path);

/*
 * 
 */
int init_filesystem(superblock_t *superblock);

#endif // INODE_H