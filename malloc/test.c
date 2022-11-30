#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "printfmt.h"


void test_realloc(void);
void test_malloc(void);
void test_calloc(void);
void test_first_fit(void);
void test_best_fit(void);

void fill_region(void *ptr, size_t size);
void *malloc_and_fill(size_t size);
void malloc_and_fill_n_regions(size_t no_regions, size_t size, void **regions);
void free_regions(void **regions);

void _test_realloc(void **ptr, size_t size, const char *str);

void
fill_region(void *_ptr, size_t size)
{
	char *ptr = (char *) _ptr;
	for (size_t i = 0; i < size; i++) {
		ptr[i] = (char) (i % 256);
	}
	for (size_t i = 0; i < size; i++) {
		assert(ptr[i] == (char) (i % 256));
	}
}

void *
malloc_and_fill(size_t size)
{
	void *ptr = malloc(size);
	fill_region(ptr, size);
	return ptr;
}

void
malloc_and_fill_n_regions(size_t no_regions, size_t size, void **regions)
{
	for (size_t i = 0; i < no_regions; i++) {
		regions[i] = malloc_and_fill(size);
	}
	regions[no_regions] = NULL;
}

void
free_regions(void **regions)
{
	while (*regions) {
		free(*regions);
		regions++;
	}
}

void
_test_realloc(void **ptr, size_t size, const char *str)
{
	strcpy(*ptr, str);
	*ptr = realloc(*ptr, size);
	if (*ptr == NULL)
		printfmt("Realloc failed to realloc ptr\n");
	assert(!strcmp(*ptr, str));
	printfmt(*ptr);
}

void
test_realloc()
{
	printfmt("===============\n");
	printfmt("REALLOC TESTING\n");
	printfmt("===============\n");

	void *ptr = realloc(NULL, 128);
	if (ptr == NULL) {
		printfmt("Realloc failed to realloc NULL ptr\n");
	}
	printfmt("Realloc works like malloc with NULL ptr\n");

	char *str = "Realloc works for bigger sizes\n";
	_test_realloc(&ptr, 256, str);

	str = "Realloc works for smaller sizes\n";
	_test_realloc(&ptr, 64, str);

	str = "Realloc works for VERY bigger sizes\n";
	_test_realloc(&ptr, 1048576, str);  // doesn't fit in a small block

	free(ptr);
	printfmt("Can free new pointer after realloc\n");
};

void
test_malloc()
{
	printfmt("==============\n");
	printfmt("MALLOC - BASIC\n");
	printfmt("==============\n");

	void *ptr;

	ptr = malloc_and_fill(1024);
	printfmt("Malloc works for small size\n");
	free(ptr);

	ptr = malloc_and_fill(65536);
	printfmt("Malloc works for medium size\n");
	free(ptr);

	ptr = malloc_and_fill(2097152);
	printfmt("Malloc works for large size\n");
	free(ptr);

	printfmt("=========================\n");
	printfmt("MALLOC - MULTIPLE\n");
	printfmt("=========================\n");

	void *regions[10];
	malloc_and_fill_n_regions(3, 1024, regions);
	malloc_and_fill_n_regions(3, 65536, regions + 3);
	malloc_and_fill_n_regions(3, 2097152, regions + 6);
	free_regions(regions);

	printfmt("Malloc works for multiple sizes\n");

	printfmt("=================================\n");
	printfmt("MALLOC - BORDERLINE CASES\n");
	printfmt("=================================\n");

	ptr = malloc(5);
	fill_region(ptr, 64);
	printfmt("When size is smaller than bigger malloc returns region with "
	         "minimum size\n");
	free(ptr);
}

void
test_calloc()
{
	printfmt("==============\n");
	printfmt("CALLOC TESTING\n");
	printfmt("==============\n");

	size_t fail = SIZE_MAX;
	int *ptr = calloc(5, fail);
	assert(ptr == NULL);
	printfmt("Calloc fails if size is too big\n");

	ptr = calloc(5, sizeof(int));
	for (int i = 0; i < 5; i++) {
		assert(ptr[i] == 0);
	}
	printfmt("Region returned by calloc is initialized with 0\n");

	fill_region(ptr, 5);
	printfmt("Can fill region saved with calloc\n");

	free(ptr);
}

void
test_first_fit()
{
	printfmt("==============\n");
	printfmt("FIRST FIT TESTING\n");
	printfmt("==============\n");

	void *original_ptr = malloc(300);
	void *ptr2 = malloc(100);
	free(original_ptr);

	void *ptr = malloc(50);
	assert(ptr == original_ptr);
	printfmt("With first fit malloc returns first available region\n");

	free(ptr);
	free(ptr2);
}

void
test_best_fit()
{
	printfmt("==============\n");
	printfmt("BEST FIT TESTING\n");
	printfmt("==============\n");

	void *big_region = malloc(256);
	void *separator = malloc(256);
	void *small_region = malloc(128);
	void *separator2 = malloc(256);

	sprintf(big_region, "big: %p\n", big_region);
	sprintf(separator, "sep: %p\n", separator);
	sprintf(small_region, "sml: %p\n", small_region);
	sprintf(separator2, "sep: %p\n", separator2);

	printfmt(big_region);
	printfmt(separator);
	printfmt(small_region);
	printfmt(separator2);

	free(big_region);
	free(small_region);
	void *res = malloc(60);
	printfmt("res: %p\n", res);

	assert(res == small_region);
	printfmt("When using best fit, smallest region available is used");
	free(separator);
	free(separator2);
}


int
main(void)
{
	test_malloc();

	test_realloc();

	test_calloc();


#ifdef FIRST_FIT
	test_first_fit();
#endif

#ifdef BEST_FIT
	test_best_fit();
#endif

	return 0;
}
