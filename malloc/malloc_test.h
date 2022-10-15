#ifndef _MALLOC_TEST_H_
#define _MALLOC_TEST_H_

struct stats_info {
	int amount_of_mallocs;
	int amount_of_frees;
	int requested_memory;
	int amount_of_mmaps;
	int amount_of_munmaps;
};

void reset_statistics();

void get_statistics(struct stats_info *test_stats);

void print_statistics();

#endif  //_MALLOC_TEST_H_
