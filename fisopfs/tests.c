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
	// PREPARATION
	system("sudo umount tests/to_mount");
	system("make clean");
	system("make");
	system("mkdir tests/bins");
	system("mkdir tests/to_mount");
	system("./fisopfs tests/to_mount/");

	// TESTING
	system("gcc tests/small_readwrite_test.c -o "
	       "tests/bins/small_readwrite_test.o");
	system("tests/bins/small_readwrite_test.o");
	system("gcc tests/medium_readwrite_test.c -o "
	       "tests/bins/medium_readwrite_test.o");
	system("tests/bins/medium_readwrite_test.o");
	system("gcc tests/large_readwrite_test.c -o "
	       "tests/bins/large_readwrite_test.o");
	system("tests/bins/large_readwrite_test.o");

	system("sudo umount tests/to_mount");
	system("rmdir tests/to_mount");
}
