#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "printfmt.h"

#define HELLO "hello from test"
#define TEST_STRING "FISOP malloc is working!"


int
main(void)
{
	printfmt("%s\n", HELLO);

	char *var0 = malloc(100);
	strcpy(var0, TEST_STRING);
	printfmt("%s\n", var0);
	char *var1 = malloc(100);
	strcpy(var1, TEST_STRING);
	printfmt("%s\n", var1);

	char *var2 = malloc(40000);
	strcpy(var2, TEST_STRING);
	printfmt("%s\n", var2);

	char *var3 = malloc(4000000);
	strcpy(var3, TEST_STRING);
	printfmt("%s\n", var3);

	free(var0);
	free(var1);
	free(var2);
	free(var3);

	var0 = malloc(100);
	strcpy(var0, TEST_STRING);
	printfmt("%s\n", var0);
	
	free(var0);

	return 0;
}
