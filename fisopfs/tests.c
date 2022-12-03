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

#define TEST_DIR "/home/julian/sisop/sisop_2022b_g02/fisopfs/tests"
#define MOUNT_DIR "/home/julian/sisop/sisop_2022b_g02/fisopfs"

int
main()
{
	system("sudo umount tests/to_mount");
	system("make clean");
	system("make");
	system("mkdir tests/to_mount");

	system("./fisopfs tests/to_mount/ ");

	chdir(TEST_DIR);

	system("gcc readwrite.c -o readwrite.o");
	system("./readwrite.o");

	chdir(MOUNT_DIR);

	system("sudo umount tests/to_mount");
	system("rmdir tests/to_mount");

	wait(NULL);
}
