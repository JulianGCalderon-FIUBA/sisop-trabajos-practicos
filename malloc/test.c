#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "printfmt.h"

#define HELLO "hello from test"
#define TEST_STRING "FISOP malloc is working!"

void
alloc_memory()
{
	char *var = malloc(100);

	strcpy(var, TEST_STRING);

	printfmt("%s\n", var);

	free(var);
}

int
main(void)
{
	printfmt("%s\n", HELLO);

	alloc_memory();
	// alloc_memory();
	// alloc_memory();

	return 0;
}
