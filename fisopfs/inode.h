#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <sys/stat.h>
#include "bitmap.h"

#define MAX_FILENAME_LENGTH 128
#define PAGES_PER_INODE 5
#define PAGE_SIZE 4096
#define INODES_PER_TABLE (PAGE_SIZE - sizeof(bitmap128_t)) / sizeof(inode_t)
#define AMOUNT_OF_INODE_TABLES 128


struct {
	ino_t     st_ino;
	nlink_t   st_nlink;
	mode_t    st_mode;
	uid_t     st_uid;
	gid_t     st_gid;
	off_t     st_size;
	time_t    st_atime;
	time_t    st_mtime;
	time_t    st_ctime;
	time_t    st_btime;
	blkcnt_t  st_blocks;
} tiny_stat;
// blksize_t st_blksize; unknown type name blksize_t

typedef char* page_t; // pointer to mem-block of size 4096

typedef struct {
	page_t *pages[PAGES_PER_INODE];
	struct tiny_stat stats;
} inode_t;

typedef struct {
	inode_t inodes[INODES_PER_TABLE];
	bitmap128_t free_inodes_bitmap;
} inode_table_t;

typedef struct {
	inode_table_t *inode_tables[AMOUNT_OF_INODE_TABLES];
	bitmap128_t free_tables_bitmap;
} superblock_t;

typedef struct {
	char name[INODES_PER_TABLE];
	int inode_id;
} dir_entry_t;

/*
 * 
 */
int get_inode_by_id(superblock_t *superblock, int inode_id, inode_t **inode);

/*
 * 
 */
int get_inode_id(superblock_t *superblock, char *path);

/*
 * 
 */
int create_inode(superblock_t *superblock); // definir args

/*
 * 
 */
int delete_inode(superblock_t *superblock, int inode_id); // si el inodo es un dir es recursivo o da error?

/*
 * 
 */
int init_filesystem(superblock_t *superblock); 

void custom_stats_to_std(struct stat *dest, const struct tiny_stat src);

#endif // INODE_H