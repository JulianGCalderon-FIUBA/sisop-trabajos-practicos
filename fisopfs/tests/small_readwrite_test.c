#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#define BUFFER_SIZE 256

int
main()
{
	system("gcc tests/small_readwrite_write.c -o "
	       "tests/bins/small_readwrite_write");
	system("gcc tests/small_readwrite_read.c -o "
	       "tests/bins/small_readwrite_read");
	system("tests/bins/small_readwrite_write > "
	       "tests/to_mount/small_readwrite_test.txt");
	system("tests/bins/small_readwrite_read < "
	       "tests/to_mount/small_readwrite_test.txt");
}
