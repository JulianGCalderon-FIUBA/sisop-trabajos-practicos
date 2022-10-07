#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "malloc.h"
#include "printfmt.h"
#include <sys/mman.h>


#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)
#define BLOCK2REGION(r) ((struct region *) ((r) + 1))
#define REGION2BLOCK(ptr) ((struct block *) (ptr) -1)


#ifndef BEST_FIT
#define FIRST_FIT
#endif

#define SUCCESS 0
#define MINIMUM_SIZE 64
#define SMALL_BLOCK_SIZE 2048
#define MEDIUM_BLOCK_SIZE 131072
#define LARGE_BLOCK_SIZE 4194304


struct region {
	bool free;
	size_t size;
	struct region *next;
	struct region *previous;
};


struct block {
	struct block *next;
	struct block *previous;
};

void split(size_t size, struct region *region);
void coalescing(struct region *left_region, struct region *right_region);
struct region *find_first_free_region_in_block(struct block *block, size_t size);
struct block **block_list_for_size(size_t size);
struct block *map_block(size_t size);
struct region *find_first_free_region_in_block_list(struct block *block_list,
                                                    size_t size);
size_t block_size_for_size(size_t size);
void free_block(struct block* block, size_t block_size);
int list_index(size_t size);

bool first_malloc = true;

struct block *small_block_list = NULL;
struct block *medium_block_list = NULL;
struct block *large_block_list = NULL;
struct block *block = NULL;


int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;
int amount_of_mmaps = 0;
int amount_of_munmaps = 0;

static void
print_statistics(void)
{
	printfmt("mallocs:   %d\n", amount_of_mallocs);
	printfmt("frees:     %d\n", amount_of_frees);
	printfmt("requested: %d\n", requested_memory);
	printfmt("mmaps: %d\n", amount_of_mmaps);
	printfmt("unmaps: %d\n", amount_of_munmaps);
}

/*
block must be valid.
if does not find free region, returns NULL
else, returns region
*/
struct region *
find_first_free_region_in_block(struct block *block, size_t size)
{
	struct region *current = BLOCK2REGION(block);

	while (!(current->free && current->size >= size)) {
		if (!current->next)
			return NULL;
		current = current->next;
	}
	return current;
}

/*
if block_list is empty, returns NULL
if does not find free region, returns NULL
else, returns region
*/
struct region *
find_first_free_region_in_block_list(struct block *block_list, size_t size)
{
	struct block *current = block_list;
	if (!current)
		return NULL;

	while (current) {
		struct region *region =
		        find_first_free_region_in_block(current, size);
		if (region)
			return region;
		current = current->next;
	}
	return NULL;
}

/*
returns corresponding index for block size */
int list_index(size_t size){
	size = block_size_for_size(size);
	switch (size)
	{
		case SMALL_BLOCK_SIZE:
			return 0;
		case MEDIUM_BLOCK_SIZE:
			return 1;
		case LARGE_BLOCK_SIZE:
			return 2;
		default:
			return 3;
	}
}
/*
finds free region for size
if does not find, returns null
else, returns region
*/
static struct region *
find_free_region(size_t size)
{
#ifdef FIRST_FIT

	struct block *block_list[] = {small_block_list, medium_block_list, large_block_list, NULL};
	int i = list_index(size);
	struct region *region;
	while (block_list[i]) {
		region = find_first_free_region_in_block_list(block_list[i], size);
		if (region)
			return region;
		i++;
	}

#endif

#ifdef BEST_FIT
	//struct region *region = find_best_free_region_in_block_list()
#endif

	return NULL;
}


/*
allocates memory for block
initializes block with single free region
*/
struct block *
map_block(size_t size)
{
	amount_of_mmaps++;
	struct block *block = mmap(
	        NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (!block)
		return NULL;
	block->next = NULL;
	block->previous = NULL;

	struct region *region = BLOCK2REGION(block);
	region->size = size - sizeof(struct block) - sizeof(struct region);
	region->next = NULL;
	region->previous = NULL;
	region->free = true;

	return block;
}

size_t
block_size_for_size(size_t size)
{
	size = size + sizeof(struct block) + sizeof(struct region);

	if (size <= SMALL_BLOCK_SIZE)
		return SMALL_BLOCK_SIZE;
	if (size <= MEDIUM_BLOCK_SIZE)
		return MEDIUM_BLOCK_SIZE;
	if (size <= LARGE_BLOCK_SIZE)
		return LARGE_BLOCK_SIZE;

	return 0;
}

/**
 * returns list of blocks for given size
 */
struct block **
block_list_for_size(size_t size)
{
	
	switch(size){
		case SMALL_BLOCK_SIZE:
			return &small_block_list;
		case MEDIUM_BLOCK_SIZE:
			return &medium_block_list;
		case LARGE_BLOCK_SIZE:
			return &large_block_list;
		default:
			return NULL;
	}
}

/*
creates new block for region of determined size
returns region
*/
static struct region *
grow_heap(size_t size)
{
	if (first_malloc) {
		atexit(print_statistics);
		first_malloc = false;
	}

	size_t block_size = block_size_for_size(size);
	if (block_size == 0)
		return NULL;

	struct block *block = map_block(block_size);
	if (!block)
		return NULL;

	printfmt("Mapping block for size %i\n", block_size);

	struct block **block_list = block_list_for_size(block_size);
	
	if (*block_list == NULL) {
		*block_list = block;
	} else {
		struct block *current = *block_list;
		while (current->next) {
			current = current->next;
		}
		current->next = block;
		block->previous = current;
	}

	return BLOCK2REGION(block);
}

/*
there should be enough space for new region
splits region in two subsequent regions
*/
void
split(size_t size, struct region *region)
{
	struct region *split_region =
	        (struct region *) ((char *) REGION2PTR(region) + size);

	split_region->size = region->size - size - sizeof(struct region);
	split_region->next = region->next;
	split_region->previous = region;
	split_region->free = true;

	region->size = size;
	region->next = split_region;
}


void *
malloc(size_t size)
{

	size = size < MINIMUM_SIZE ? MINIMUM_SIZE : size;
	struct region *region;

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);

	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	region = find_free_region(size);

	if (!region)
		region = grow_heap(size);
	if (!region)
		return NULL;

	if (region->size >= size + sizeof(struct region) + MINIMUM_SIZE)
		split(size, region);

	region->free = false;

	return REGION2PTR(region);
}

/*
combines two regions into single region
*/
void
coalescing(struct region *left_region, struct region *right_region)
{
	left_region->next = right_region->next;
	if(right_region->next)
		right_region->next->previous = left_region;

	left_region->size += sizeof(struct region) + right_region->size;

}

void
free_block(struct block* block, size_t block_size)
{	
	struct block *previous_block = block->previous;
	struct block *next_block = block->next;
	struct block **block_list = block_list_for_size(block_size);
	
	amount_of_munmaps++;
	int status = munmap(block, block_size);
	if (status == SUCCESS) {
		if (!previous_block)
			*block_list = next_block;
		else
			previous_block->next = next_block;
		
		if (next_block)
			next_block->previous = previous_block;
	} else {
		perror("Free failed (munmap error)");
	}
}

void
free(void *ptr)
{
	// updates statistics
	amount_of_frees++;

	struct region *region = PTR2REGION(ptr);
	assert(region->free == 0);
	region->free = true;

	if (region->previous && region->previous->free) {
		coalescing(region->previous, region);
		region = region->previous;
	}
	if (region->next && region->next->free) 
		coalescing(region, region->next);

	if (!region->previous && !region->next) {
		struct block *block = REGION2BLOCK(region);
		size_t block_size = region->size + sizeof(struct region) +
							sizeof(struct block);

		printfmt("freeing block of size %i\n", block_size);
							
		free_block(block, block_size);
	}
}

// void *
// calloc(size_t nmemb, size_t size)
// {
// 	return NULL;
// }

// void *
// realloc(void *ptr, size_t size)
// {
// 	return NULL;
// }
