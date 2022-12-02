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
	int write_buffer[BUFFER_SIZE];
	for (int i = 0; i < BUFFER_SIZE; i++) {
		write_buffer[i] = i;
	}

	fwrite(write_buffer, sizeof(int), BUFFER_SIZE, stdout);
}
