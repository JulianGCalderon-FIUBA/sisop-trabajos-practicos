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
	system("gcc tests/medium_readwrite_write.c -o "
	       "tests/bins/medium_readwrite_write");
	wait(NULL);
	system("gcc tests/medium_readwrite_read.c -o "
	       "tests/bins/medium_readwrite_read");
	wait(NULL);
	system("tests/bins/medium_readwrite_write > "
	       "tests/to_mount/medium_readwrite_test.txt");
	wait(NULL);
	system("tests/bins/medium_readwrite_read < "
	       "tests/to_mount/medium_readwrite_test.txt");
	wait(NULL);
}
