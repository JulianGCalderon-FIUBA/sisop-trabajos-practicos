#include <inc/lib.h>

void mark_n(int n);

#define BLUE "\e[0;34m"
#define RESET "\e[0m"
#define RED "\e[0;31m"

void
mark_n(int n)
{
	int eid = sys_getenvid();

	for (int i = 0; i < n; i++) {
		cprintf(BLUE "[%d] yield\n" RESET, eid);
		sys_yield();
	}
}

void
umain(int argc, char **argv)
{
	mark_n(5);

	int eid = sys_getenvid();
	cprintf(RED "[%d] forking\n" RESET, eid);

	if (!fork()) {
		mark_n(5);
	} else {
		mark_n(5);
	}
}
