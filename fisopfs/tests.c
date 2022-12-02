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
	// PREPARATION
	system("make clean");
	system("make");
	system("mkdir tests/to_mount");
	system("./fisopfs tests/to_mount/");

	// TESTING
	system("gcc tests/small_readwrite_test.c -o "
	       "tests/bins/small_readwrite_test");
	system("tests/bins/small_readwrite_test");
	system("gcc tests/medium_readwrite_test.c -o "
	       "tests/bins/medium_readwrite_test");
	system("tests/bins/medium_readwrite_test");

	system("sudo umount tests/to_mount");
	system("rmdir tests/to_mount");
}
