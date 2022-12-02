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
	printf("TESTING TO WRITE 1024 BYTES\n");


	system("gcc tests/small_readwrite_write.c -o "
	       "tests/bins/small_readwrite_write.o");
	system("gcc tests/small_readwrite_read.c -o "
	       "tests/bins/small_readwrite_read.o");
	system("tests/bins/small_readwrite_write.o > "
	       "tests/to_mount/small_readwrite_test.txt");
	system("tests/bins/small_readwrite_read.o < "
	       "tests/to_mount/small_readwrite_test.txt");

	printf("OK\n");
}
