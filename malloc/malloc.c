#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "malloc.h"
#include "printfmt.h"
#include <sys/mman.h>

#define ALIGN4(s) (((((s) -1) >> 2) << 2) + 4)
#define REGION2PTR(r) ((r) + 1)
#define PTR2REGION(ptr) ((struct region *) (ptr) -1)

#ifndef BEST_FIT
#define FIRST_FIT
#endif

#define SIZE_MINIMUM 64
#define SMALL_BLOCK_SIZE 2048
#define MEDIUM_BLOCK_SIZE 131072
#define LARGE_BLOCK_SIZE 4194304

struct region {
	bool free;
	size_t size;
	struct region *next;
	struct region *previous;
};

void split(size_t size, struct region *region);
void coalescing(struct region *left_region, struct region *right_region);

struct block {
	struct block *next;
	struct block *previous;
};

struct block *small_block_list = NULL;
struct block *medium_block_list = NULL;
struct block *large_block_list = NULL;

struct block *block = NULL;

int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;
int amount_of_mmap = 0;
int amount_of_munmap = 0;

static void
print_statistics(void)
{
	printfmt("mallocs:   %d\n", amount_of_mallocs);
	printfmt("frees:     %d\n", amount_of_frees);
	printfmt("requested: %d\n", requested_memory);
	printfmt("mmap: %d\n", amount_of_mmap);
	printfmt("unmap: %d\n", amount_of_munmap);
}

struct region *
find_free_region_in_block(struct block *block, size_t size)
{
	struct region *current = (struct region *) (block + 1);

	while (!(current->free && current->size >= size)) {
		if (!current->next)
			return NULL;
		current = current->next;
	}
	return current;
}

// finds the next free region
// that holds the requested size
//
static struct region *
find_free_region(size_t size)
{
#ifdef FIRST_FIT


	struct block *current = small_block_list;
	if (!current)
		return NULL;

	while (current) {
		struct region *region = find_free_region_in_block(current, size);
		if (region)
			return region;
		current = current->next;
	}
	return NULL;

#endif

#ifdef BEST_FIT
	// Your code here for "best fit"
#endif
}

struct block *
map_block(size_t size)
{
	amount_of_mmap++;
	struct block *block = mmap(
	        NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (!block)
		return NULL;
	block->next = NULL;
	block->previous = NULL;

	struct region *region =
	        (struct region *) ((char *) block + sizeof(block));
	region->size = size - sizeof(block) - sizeof(region);
	region->next = NULL;
	region->previous = NULL;
	region->free = true;

	return block;
}

static struct region *
grow_heap(size_t size)
{
	puts("empezando grow_heap");
	struct block *block = map_block(SMALL_BLOCK_SIZE);
	if (!block)
		return NULL;

	if (!small_block_list) {
		small_block_list = block;
		atexit(print_statistics);
	} else {
		struct block *current = small_block_list;
		while (current->next) {
			current = current->next;
		}
		current->next = block;
		block->previous = current;
	}
	puts("terminando grow_heap");
	return (struct region *) (block + 1);
}

void
split(size_t size, struct region *region)
{
	struct region *split_region =
	        (struct region *) ((char *) region + size + sizeof(region));

	split_region->size = region->size - sizeof(split_region) - size;
	split_region->next = region->next;
	split_region->previous = region;
	split_region->free = true;

	region->size = size;
	region->next = split_region;
	region->free = false;
}

void *
malloc(size_t size)
{
	size = size < SIZE_MINIMUM ? SIZE_MINIMUM : size;
	struct region *next;

	// aligns to multiple of 4 bytes
	size = ALIGN4(size);

	// updates statistics
	amount_of_mallocs++;
	requested_memory += size;

	write(2, "empezando free_region\n", 23);
	next = find_free_region(size);
	write(2, "terminando free_region\n", 23);

	if (!next) {
		next = grow_heap(size);
	}

	if (next->size > size + sizeof(next))
		split(size, next);


	return REGION2PTR(next);
}

void
coalescing(struct region *left_region, struct region *right_region)
{
	left_region->next = right_region->next;
	left_region->size += sizeof(right_region) + right_region->size;
}

void
free(void *ptr)
{
	// updates statistics
	amount_of_frees++;

	struct region *curr = PTR2REGION(ptr);
	assert(curr->free == 0);

	curr->free = true;

	if (curr->previous && curr->previous->free) {
		coalescing(curr->previous, curr);
	}
	if (curr->next && curr->next->free) {
		coalescing(curr, curr->next);
	}
	/*
	        if (!(curr->previous && curr->next)) {
	                struct block *block =
	                        (struct block *) ((char *) curr - sizeof(struct block));

	                if (block->previous)
	                        block->previous->next = block->next;
	                if (block->next)
	                        block->next->previous = block->previous;

	                amount_of_munmap++;
	                int status =
	                        munmap(block, curr->size + sizeof(curr) + sizeof(block));
	                if (status == -1) {
	                        perror("Free failed (munmap error)");
	                }
	        }*/
}

void *
calloc(size_t nmemb, size_t size)
{
	// Your code here

	return NULL;
}

void *
realloc(void *ptr, size_t size)
{
	// Your code here

	return NULL;
}
