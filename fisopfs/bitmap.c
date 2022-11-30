#include <limits.h>
#include "bitmap.h"

/*
 * Sets all the bits to 0
 */
void
bitmap_set_all_0(bitmap128_t *bitmap)
{
	bitmap->bitmap64_0 = 0;
	bitmap->bitmap64_1 = 0;
}

/*
 * Sets all the bits to 1
 */
void
bitmap_set_all_1(bitmap128_t *bitmap)
{
	bitmap->bitmap64_0 = ~0;
	bitmap->bitmap64_1 = ~0;
}

/*
 * Returns the 64bit bitmap from the 128bit bitmap where "pos" points to.
 * Also updates "pos" so that it points to a bit within the bitmap64 (pos %= 64)
 */
static uint64_t *
get_bitmap64_by_pos(bitmap128_t *bitmap, int *pos)
{
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
int
bitmap_getbit(bitmap128_t *bitmap, int pos)
{
	uint64_t bitmap64 = *get_bitmap64_by_pos(bitmap, &pos);
	uint64_t mask = ~(ULLONG_MAX - 1);  // 0x00...01
	return (int) (bitmap64 >> (63 - pos)) & mask;
}

/*
 * Set bit in position "pos" to 1
 */
void
bitmap_setbit(bitmap128_t *bitmap, int pos)
{
	uint64_t *bitmap64 = get_bitmap64_by_pos(bitmap, &pos);
	uint64_t mask = 1;
	mask <<= (63 - pos);
	*bitmap64 |= mask;
}

/*
 * Set bit in position "pos" to 0
 */
void
bitmap_clearbit(bitmap128_t *bitmap, int pos)
{
	uint64_t *bitmap64 = get_bitmap64_by_pos(bitmap, &pos);
	uint64_t mask = 1;
	mask <<= (63 - pos);
	*bitmap64 &= ~mask;
}

/*
 * Get the position of the leftmost bit that has a value of 1
 * Returns -1 if all bits are set to 0
 */
int
bitmap_count_leading_zeros(bitmap128_t *bitmap)
{
	if (bitmap->bitmap64_0 != 0)
		return __builtin_clzl(bitmap->bitmap64_0);
	if (bitmap->bitmap64_1 != 0)
		return __builtin_clzl(bitmap->bitmap64_1) + 64;
	return -1;
}

/*
 * Returns true if at least one bit is set to 1, else returns false
 * Equivalent to !!bitmap
 */
bool
bitmap_has_set_bit(bitmap128_t *bitmap)
{
	return bitmap->bitmap64_0 || bitmap->bitmap64_1;
}

/*
 * Returns true if at least one bit is set to 0
 * Equivalent to !!~bitmap
 */
bool
bitmap_has_unset_bit(bitmap128_t *bitmap)
{
	return ~bitmap->bitmap64_0 || ~bitmap->bitmap64_1;
}
