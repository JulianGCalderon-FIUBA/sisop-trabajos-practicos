#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint64_t bitmap64_0;
	uint64_t bitmap64_1;
} bitmap128_t;

/**
 * Sets all the bits to 0
 */
void bitmap_set_all_0(bitmap128_t *bitmap);

/**
 * Sets all the bits to 1
 */
void bitmap_set_all_1(bitmap128_t *bitmap);

/**
 * Get value stored in position "pos"
 */
int bitmap_getbit(bitmap128_t *bitmap, int pos);

/**
 * Set bit in position "pos" to 1
 */
void bitmap_setbit(bitmap128_t *bitmap, int pos);

/**
 * Set bit in position "pos" to 0
 */
void bitmap_clearbit(bitmap128_t *bitmap, int pos);

/**
 * Get the position of the leftmost bit that has a value of 1
 * Returns -1 if all bits are set to 0
 */
int bitmap_count_leading_zeros(bitmap128_t *bitmap);

/**
 * Returns true if at least one bit is set to 1, else returns false
 * Equivalent to !!bitmap
 */
bool bitmap_has_set_bit(bitmap128_t *bitmap);

/**
 * Returns true if at least one bit is set to 0
 * Equivalent to !!~bitmap
 */
bool bitmap_has_unset_bit(bitmap128_t *bitmap);

#endif  // BITMAP_H
