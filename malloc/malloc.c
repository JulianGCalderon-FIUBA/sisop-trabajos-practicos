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
finds free region for size
if does not find, returns null
else, returns region
*/
static struct region *
find_free_region(size_t size)
{
#ifdef FIRST_FIT

	struct region *region =
	        find_first_free_region_in_block_list(small_block_list, size);
	if (region) {
		return region;
	}

#endif

#ifdef BEST_FIT
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

/*
creates new block for region of determined size
returns region
*/
static struct region *
grow_heap(size_t size)
{
	struct block *block = map_block(SMALL_BLOCK_SIZE);
	if (!block)
		return NULL;

	// SI SE LIBERA EL BLOQUE CHICO VOLVERA A LLAMAR A ATEXIT
	// DEFINIMOS VARIABLE GLOBAL PARA INDICAR PRIMER LLAMADO?
	// LO PODEMOS HACER EN MALLOC, CREO QUE SERIA MAS PROLIJO
	if (!small_block_list) {
		atexit(print_statistics);
		small_block_list = block;
	} else {
		struct block *current = small_block_list;
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

	if (region->size >= size + sizeof(struct region) + MINIMUM_SIZE)
		split(size, region);

	region->free = false;

	return REGION2PTR(region);
}

/**
 * returns list of blocks for given size
 */
struct block **
block_list_for_size(size_t size)
{
	if (size <= SMALL_BLOCK_SIZE)
		return &small_block_list;
	if (size <= MEDIUM_BLOCK_SIZE)
		return &medium_block_list;
	if (size <= LARGE_BLOCK_SIZE)
		return &large_block_list;

	return NULL;
}

/*
combines two regions into single region
*/
void
coalescing(struct region *left_region, struct region *right_region)
{
	left_region->next = right_region->next;
	left_region->size += sizeof(struct region) + right_region->size;
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
	}
	if (region->next && region->next->free) {
		coalescing(region, region->next);
	}

	if (!region->previous && !region->next) {
		struct block *block = REGION2BLOCK(region);

		if (block->previous)
			block->previous->next = block->next;
		if (block->next)
			block->next->previous = block->previous;

		size_t mapped_size = region->size + sizeof(struct region) +
		                     sizeof(struct block);


		struct block **block_list = block_list_for_size(mapped_size);
		if (!block->previous) {
			*block_list = block->next;
		}

		amount_of_munmaps++;
		int status = munmap(block, mapped_size);
		if (status == -1) {
			// SI FALLA, EL BLOQUE VA A ESTAR DESCONECTADO DE LA LISTA.
			perror("Free failed (munmap error)");
		}
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
