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
	system("sudo umount tests/to_mount");
	system("make clean");
	system("make");
	system("mkdir tests/to_mount");

	if (fork() == 0) {
		system("./fisopfs tests/to_mount/ -f");
		return 0;
	}

	chdir("tests");

	system("gcc readwrite.c -o readwrite.o");
	system("./readwrite.o");

	chdir("..");

	system("sudo umount tests/to_mount");
	system("rmdir tests/to_mount");

	wait(NULL);
}
