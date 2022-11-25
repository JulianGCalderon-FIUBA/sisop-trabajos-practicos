#include <stdint.h>


#define MAX_FILENAME_LENGTH 128
#define PAGES_PER_INODE 5
#define INODES_PER_TABLE 51
#define AMOUNT_OF_INODE_TABLES 128
#define PAGE_SIZE 4096

typedef uint8_t[4096] page_t; // pointer to mem-block of size 4096

typedef struct {
	ssize_t read_date; // in epoch format, upto miliseconds
	ssize_t birth_date; // in epoch format, upto miliseconds
	ssize_t modify_date; // in epoch format, upto miliseconds
	page_t *pages[PAGES_PER_INODE];
	unsigned int owner_uid;
	unsigned int owner_gid;
	unsigned small permissions;
	unsigned small content_size;
	enum inode_type{FILE, DIR, LINK} type;
} inode_t;

typedef struct {
	inode_t *inodes[INODES_PER_TABLE];
	bitmap128_t free_inodes_bitmap;
} inode_table_t;

typedef struct {
	inode_table_t *tables[AMOUNT_OF_INODE_TABLES];
	bitmap128_t free_tables_bitmap;
} superblock_t;

typedef struct {
	char name[INODES_PER_TABLE];
	bitmap128_t free_inodes_bitmap;
} dir_entry_t;

/*
 * 
 */
int get_inode_by_id(superblock_t *superblock, int inode_id, inode_t *inode);

/*
 * 
 */
int get_inode_id(superblock_t *superblock, char *path);

/*
 * 
 */
int create_inode(superblock_t *superblock, ); // definir args

/*
 * 
 */
int delete_inode(superblock_t *superblock, int inode_id); // si el inodo es un dir es recursivo o da error?

/*
 * 
 */
int init_inodes(superblock_t *superblock); 
