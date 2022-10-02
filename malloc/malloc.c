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

#define SIZE_MINIMUM 64
#define SMALL_BLOCK_SIZE 2048

struct region {
	bool free;
	size_t size;
	struct region *next;
	struct region *previous;
};

void split(size_t size, struct region *region);
void coalescing(struct region *left_region, struct region *right_region);

struct block {
	struct region *region_list;
};

struct block *block = NULL;
struct region *region_list = NULL;

int amount_of_mallocs = 0;
int amount_of_frees = 0;
int requested_memory = 0;

static void
print_statistics(void)
{
	printfmt("mallocs:   %d\n", amount_of_mallocs);
	printfmt("frees:     %d\n", amount_of_frees);
	printfmt("requested: %d\n", requested_memory);
}

// finds the next free region
// that holds the requested size
//
static struct region *
find_free_region(size_t size)
{
	struct region *current = region_list;
	if (!current)
		return NULL;

	while (!(current->free && current->size >= size)) {
		if (!current->next)
			return NULL;
		current = current->next;
	}

#ifdef FIRST_FIT
	// Your code here for "first fit"
#endif

#ifdef BEST_FIT
	// Your code here for "best fit"
#endif

	return current;
}

static struct region *
grow_heap(size_t size)
{
	/*
	// finds the current heap break
	struct region *curr = (struct region *) sbrk(0);

	// allocates the requested size
	struct region *prev =
	        (struct region *) sbrk(sizeof(struct region) + size);

	// verifies that the returned address
	// is the same that the previous break
	// (ref: sbrk(2))
	assert(curr == prev);

	// verifies that the allocation
	// is successful
	//
	// (ref: sbrk(2))
	if (curr == (struct region *) -1) {
	        return NULL;
	}

	curr->size = size;
	curr->next = NULL;
	curr->free = false;

	return curr;*/
	// first time here
	if (region_list) {
		exit(EXIT_FAILURE);
	}
	struct region *region = mmap(NULL,
	                             SMALL_BLOCK_SIZE,
	                             PROT_READ | PROT_WRITE,
	                             MAP_ANON | MAP_PRIVATE,
	                             -1,
	                             0);

	region->size = SMALL_BLOCK_SIZE - sizeof(region);
	region->next = NULL;
	region->previous = NULL;
	region->free = true;

	region_list = region;
	atexit(print_statistics);

	return region;
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

	next = find_free_region(size);

	if (!next) {
		next = grow_heap(size);
	}

	if (next->size > size + sizeof(next))
		split(size, next);

	// Your code here
	//
	// hint: maybe split free regions?

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

	if (curr->previous && curr->previous->free && curr->free) {
		coalescing(curr->previous, curr);
	}
	if (curr->next && curr->next->free && curr->free) {
		coalescing(curr, curr->next);
	}


	// Your code here
	//
	// hint: maybe coalesce regions?
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
