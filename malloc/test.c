#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "printfmt.h"

#define HELLO "hello from test"
#define TEST_STRING "FISOP malloc is working!"

void 
test_realloc(
	char *ptr = realloc(NULL, 128);
	if (ptr == NULL) {
		printfmt("Realloc failed to realloc NULL ptr\n");
	}
	strcpy(ptr, "Realloc works for bigger size\n");
	realloc(ptr, 256);
	puts(ptr);

	strcpy(ptr, "Realloc works for smaller size\n");
	realloc(ptr, 64);
	puts(ptr);

	strcpy(ptr, "Realloc works for VERY bigger sizes\n");
	realloc(ptr, 1048576); // doesn't fit in a small block
	puts(ptr);
);

int
main(void)
{
	printfmt("%s\n", HELLO);

	// char *var0 = malloc(100);
	// strcpy(var0, TEST_STRING);
	// printfmt("%s\n", var0);
	// char *var1 = malloc(100);
	// strcpy(var1, TEST_STRING);
	// printfmt("%s\n", var1);

	// char *var2 = malloc(40000);
	// strcpy(var2, TEST_STRING);
	// printfmt("%s\n", var2);

	char *var3 = malloc(130000);
		  var3 = realloc(var3,100);
	strcpy(var3, TEST_STRING);
	printfmt("%s\n", var3);

	// free(var0);
	// free(var1);
	// free(var2);
	free(var3);

	// var0 = malloc(100);
	// strcpy(var0, TEST_STRING);
	// printfmt("%s\n", var0);
	
	// free(var0);

	return 0;
}
