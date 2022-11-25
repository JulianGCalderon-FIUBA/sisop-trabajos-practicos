#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

typedef struct {
	uint64_t bitmap64_0;
	uint64_t bitmap64_1;
} bitmap128_t;

/*
 * Sets all the bits to 0
 */
void bitmap_set_all_0(bitmap128_t *bitmap);

/*
 * Sets all the bits to 1
 */
void bitmap_set_all_1(bitmap128_t *bitmap);

/*
 * Get value stored in position "pos"
 */
int bitmap_getbit(bitmap128_t *bitmap, int pos);

/*
 * Set bit in position "pos" to 1
 */
void bitmap_setbit(bitmap128_t *bitmap, int pos);

/*
 * Set bit in position "pos" to 0
 */
void bitmap_clearbit(bitmap128_t *bitmap, int pos);

/*
 * Get the position of the leftmost bit that has a value of 1
 */
int bitmap_most_significant_bit(bitmap128_t *bitmap);

#endif // BITMAP_H
