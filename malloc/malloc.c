#define _DEFAULT_SOURCE

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "malloc.h"
#include "printfmt.h"
#include <sys/mman.h>

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)
#define BLOCK2REGION(r) ((struct region *) ((r) + 1))
#define REGION2BLOCK(ptr) ((struct block *) (ptr) -1)

#ifndef BEST_FIT
#ifndef FIRST_FIT

#define FIRST_FIT

#endif
#endif

#define SUCCESS 0
#define MINIMUM_SIZE 64
#define SMALL_BLOCK_SIZE 2048
#define MEDIUM_BLOCK_SIZE 131072
#define LARGE_BLOCK_SIZE 4194304

struct region {
	void *magic;
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
struct block *map_block(size_t size);
struct region *find_first_free_region_in_block_list(struct block *block_list,
                                                    size_t size);
size_t block_size_for_size(size_t size);
void free_block(struct block *block, size_t block_size);
int list_index(size_t size);
struct region *find_best_free_region_in_block_list(struct block *block,
                                                   size_t requested_size);
struct region *find_best_free_region_in_block(struct block *block,
                                              size_t requested_size);

void print_statistics(void);

bool first_malloc = true;

struct block *small_block_list = NULL;
struct block *medium_block_list = NULL;
struct block *large_block_list = NULL;
struct block *block = NULL;

size_t block_size_list[] = {
	SMALL_BLOCK_SIZE, MEDIUM_BLOCK_SIZE, LARGE_BLOCK_SIZE, 0
};

struct block **block_lists[] = {
	&small_block_list, &medium_block_list, &large_block_list, NULL
};

int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;
int amount_of_mmaps = 0;
int amount_of_munmaps = 0;

void
print_statistics()
{
	printfmt("== STATISTICS ==\n");
	printfmt("mallocs:   %d\n", amount_of_mallocs);
	printfmt("frees:     %d\n", amount_of_frees);
	printfmt("requested: %d\n", requested_memory);
	printfmt("mmaps: %d\n", amount_of_mmaps);
	printfmt("unmaps: %d\n", amount_of_munmaps);
}

/*
returns nearest block size for given size
*/
size_t
block_size_for_size(size_t size)
{
	size = size + sizeof(struct block) + sizeof(struct region);

	for (int i = 0; block_size_list[i]; i++) {
		if (size <= block_size_list[i]) {
			return block_size_list[i];
		}
	}
	return 0;
}

/*
returns index corresponding to block list for given size
*/
int
list_index(size_t size)
{
	int i = 0;

	for (; block_size_list[i]; i++) {
		if (size == block_size_list[i]) {
			return i;
		}
	}
	return i;
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

struct region *
find_best_free_region_in_block(struct block *block, size_t requested_size)
{
	struct region *best = NULL;
	size_t best_region_size = LARGE_BLOCK_SIZE;
	struct region *region = BLOCK2REGION(block);
	while (region) {
		if (region->size >= requested_size &&
		    region->size < best_region_size && region->free) {
			best_region_size = region->size;
			best = region;
		}
		region = region->next;
	}
	return best;
}

struct region *
find_best_free_region_in_block_list(struct block *block, size_t requested_size)
{
	struct region *best_region = NULL;
	size_t best_region_size = LARGE_BLOCK_SIZE;
	struct region *region;
	while (block) {
		region = find_best_free_region_in_block(block, requested_size);
		if (region && region->size < best_region_size) {
			best_region_size = region->size;
			best_region = region;
		}
		block = block->next;
	}
	return best_region;
}

/*
finds free region for size
if does not find, returns null
else, returns region
*/
static struct region *
find_free_region(size_t size)
{
	struct region *region;

#ifdef FIRST_FIT
	for (int i = list_index(block_size_for_size(size)); block_lists[i]; i++) {
		region = find_first_free_region_in_block_list(*block_lists[i],
		                                              size);
		if (region)
			return region;
	}
	return NULL;

#endif

#ifdef BEST_FIT
	struct region *best = NULL;
	size_t best_region_size = LARGE_BLOCK_SIZE;
	for (int i = list_index(block_size_for_size(size)); block_lists[i]; i++) {
		region = find_best_free_region_in_block_list(*block_lists[i],
		                                             size);
		if (region && region->size < best_region_size) {
			best_region_size = region->size;
			best = region;
		}
	}
	return best;
#endif
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
	region->magic = REGION2PTR(region);
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

	struct block **block_list = block_lists[list_index(block_size)];

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
	split_region->magic = REGION2PTR(split_region);

	if (region->next)
		region->next->previous = split_region;

	region->size = size;
	region->next = split_region;

	if (split_region->next && split_region->next->free) {
		coalescing(split_region, split_region->next);
	}
}

/*
returns free region with given size
on error returns NULL
*/
void *
malloc(size_t size)
{
	if (size == 0)
		return NULL;
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
	if (!region) {
		errno = ENOMEM;
		return NULL;
	}

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
	if (right_region->next)
		right_region->next->previous = left_region;

	left_region->size += sizeof(struct region) + right_region->size;
}

/*
frees given block
on error does nothing
*/
void
free_block(struct block *block, size_t block_size)
{
	struct block *previous_block = block->previous;
	struct block *next_block = block->next;
	struct block **block_list = block_lists[list_index(block_size)];


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

/*
frees region ptr points to
panics if region is already free
*/
void
free(void *ptr)
{
	if (!ptr)
		return;

	// updates statistics
	amount_of_frees++;

	struct region *region = PTR2REGION(ptr);
	if (region->magic != ptr || region->free)
		return;
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

		free_block(block, block_size);
	}
}

void *
calloc(size_t nmemb, size_t size)
{
	if (size == 0 || nmemb == 0)
		return NULL;
	if (nmemb > INT_MAX / size) {
		perror("Overflow in calloc");
		return NULL;
	}

	size_t total_size = nmemb * size;
	struct region *region = malloc(total_size);
	if (!region)
		return NULL;
	region = memset(region, 0, total_size);

	return region;
}

void *
realloc(void *ptr, size_t size)
{
	if (size >
	    LARGE_BLOCK_SIZE - sizeof(struct block) - sizeof(struct region)) {
		errno = ENOMEM;
		return NULL;
	}

	if (ptr == NULL)
		return malloc(size);

	if (size == 0) {
		free(ptr);
		return NULL;
	}

	struct region *region = PTR2REGION(ptr);

	long int signed_offset = size - region->size;
	if (signed_offset <= 0) {
		if (region->size >= size + sizeof(struct region) + MINIMUM_SIZE)
			split(size, region);
		return ptr;
	}
	size_t size_offset = (size_t) signed_offset;
	if (region->next && region->next->free &&
	    region->next->size + sizeof(struct region) >= size_offset) {
		coalescing(region, region->next);

		if (region->size >= size + sizeof(struct region) + MINIMUM_SIZE)
			split(size, region);
		return ptr;
	}

	void *res = malloc(size);
	memcpy(res, ptr, (PTR2REGION(ptr))->size);
	free(ptr);
	return res;
}
