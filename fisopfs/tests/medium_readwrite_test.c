#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>


int
main()
{
	system("gcc tests/medium_readwrite_write.c -o "
	       "tests/bins/medium_readwrite_write");
	system("gcc tests/medium_readwrite_read.c -o "
	       "tests/bins/medium_readwrite_read");
	system("tests/bins/medium_readwrite_write > "
	       "tests/to_mount/medium_readwrite_test.txt");
	system("tests/bins/medium_readwrite_read < "
	       "tests/to_mount/medium_readwrite_test.txt");
}
