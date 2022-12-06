#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


void
fill_buffer(int *buffer, int size)
{
	for (int i = 0; i < size; i++) {
		buffer[i] = i;
	}
}

void
compare_buffers(int *buffer1, int *buffer2, int size)
{
	for (int i = 0; i < size; i++) {
		if (buffer1[i] != buffer2[i]) {
			printf("expected %i, got %i", buffer1[i], buffer2[i]);
			exit(-1);
		}
	}
}

void
readwrite_test(size_t size)
{
	int write_buffer[size];
	fill_buffer(write_buffer, size);

	FILE *file = fopen("to_mount/readwrite.txt", "w+");

	fwrite(write_buffer, sizeof(int), size, file);
	fseek(file, 0, SEEK_SET);

	int read_buffer[size];
	fread(read_buffer, sizeof(int), size, file);

	compare_buffers(write_buffer, read_buffer, size);

	fclose(file);
}

void
readwrite_tobig_test()
{
	int write_buffer[6143];
	fill_buffer(write_buffer, 6143);

	FILE *file = fopen("to_mount/readwrite_tobig.txt", "w+");

	size_t written = fwrite(write_buffer, sizeof(int), 6000, file);

	fclose(file);

	assert(strcmp(strerror(errno), "File too large") == 0);
}


int
main()
{
	printf("1024 readwrite\n");
	readwrite_test(1024);
	printf("ok\n");

	printf("4096 readwrite\n");
	readwrite_test(4096);
	printf("ok\n");

	printf("5120 readwrite\n");
	readwrite_test(5120);
	printf("ok\n");

	printf("tobig readwrite\n");
	readwrite_tobig_test();
	printf("ok\n");

	return 0;
}
