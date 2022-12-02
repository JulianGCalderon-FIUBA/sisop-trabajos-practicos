#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>


int
main()
{
	printf("TESTING TO WRITE 20480 BYTES\n");

	system("gcc tests/large_readwrite_write.c -o "
	       "tests/bins/large_readwrite_write");
	system("gcc tests/large_readwrite_read.c -o "
	       "tests/bins/large_readwrite_read");
	system("tests/bins/large_readwrite_write > "
	       "tests/to_mount/large_readwrite_test.txt");
	system("tests/bins/large_readwrite_read < "
	       "tests/to_mount/large_readwrite_test.txt");

	printf("OK\n");
}
