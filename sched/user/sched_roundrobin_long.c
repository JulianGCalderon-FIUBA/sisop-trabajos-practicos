#include <inc/lib.h>
#define BLUE "\e[0;34m"
#define RESET "\e[0m"
#define RED "\e[0;31m"


void
umain(int argc, char **argv)
{
	int eid = sys_getenvid();
	for (int i = 0; i < 75; i++) {
		cprintf(BLUE "[%d] yield\n" RESET, eid);
		sys_yield();
	}
}
