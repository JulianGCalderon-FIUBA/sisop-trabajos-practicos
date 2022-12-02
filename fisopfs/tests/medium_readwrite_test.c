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
	printf("TESTING TO WRITE 16384 BYTES\n");


	system("gcc tests/medium_readwrite_write.c -o "
	       "tests/bins/medium_readwrite_write.o");
	system("gcc tests/medium_readwrite_read.c -o "
	       "tests/bins/medium_readwrite_read.o");
	system("tests/bins/medium_readwrite_write.o > "
	       "tests/to_mount/medium_readwrite_test.txt");
	system("tests/bins/medium_readwrite_read.o < "
	       "tests/to_mount/medium_readwrite_test.txt");

	printf("OK\n");
}
