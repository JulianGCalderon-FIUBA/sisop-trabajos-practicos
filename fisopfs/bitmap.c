#include "bitmap.h"

/*
 * Sets all the bits to 0
 */
void bitmap_reset(bitmap128_t *bitmap) {
	bitmap->bitmap64_0 = 0;
	bitmap->bitmap64_1 = 0;
}

/*
 * Returns the 64bit bitmap from the 128bit bitmap where "pos" points to.
 * Also updates "pos" so that it points to a bit within the bitmap64 (pos %= 64)
 */
static uint64_t *get_bitmap64_by_pos(bitmap128_t *bitmap, int *pos) {
	uint64_t *bitmap64;
	if (*pos < 64) {
		bitmap64 = &bitmap->bitmap64_0;
	} else {
		bitmap64 = &bitmap->bitmap64_1;
		*pos -= 64;
	}
	return bitmap64;
}

/*
 * Get value stored in position "pos". Either 0 or 1
 */
int bitmap_getbit(bitmap128_t *bitmap, int pos) {
	uint64_t bitmap64 = *get_bitmap64_by_pos(bitmap, &pos);
	return (int) bitmap64 << pos >> 63 - pos;
}

/*
 * Set bit in position "pos" to 1
 */
void bitmap_setbit(bitmap128_t *bitmap, int pos) {
	uint64_t *bitmap64 = get_bitmap64_by_pos(bitmap, &pos);
	uint64_t mask = 1 << 63 - pos;
	*bitmap64 |= mask;
}

/*
 * Set bit in position "pos" to 0
 */
void bitmap_clearbit(bitmap128_t *bitmap, int pos) {
	uint64_t *bitmap64 = get_bitmap64_by_pos(bitmap, &pos);
	uint64_t mask = 1 << 63 - pos;
	*bitmap64 &= ~ mask;
}

/*
 * Get the position of the leftmost bit that has a value of 1
 * Returns -1 if all bits are set to 0
 */
int bitmap_count_leading_zeros(bitmap128_t *bitmap) {
	if (bitmap->bitmap64_0 != 0) {
		return __builtin_clzll(bitmap->bitmap64_0);
	} else if (bitmap->bitmap64_1 != 0) {
		return __builtin_clzll(bitmap->bitmap64_1) + 64;
	}
	return -1;
}
