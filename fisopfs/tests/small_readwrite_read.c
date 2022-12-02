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
	int read_buffer[BUFFER_SIZE];

	fread(read_buffer, sizeof(int), BUFFER_SIZE, stdin);

	for (int i = 0; i < BUFFER_SIZE; i++) {
		assert(read_buffer[i] == i);
	}
}
