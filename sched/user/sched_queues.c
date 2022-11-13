// Concurrent version of prime sieve of Eratosthenes.
// Invented by Doug McIlroy, inventor of Unix pipes.
// See http://swtch.com/~rsc/thread/.
// The picture halfway down the page and the text surrounding it
// explain what's going on here.
//
// Since NENVS is 1024, we can print 1022 primes before running out.
// The remaining two environments are the integer generator at the bottom
// of main and user/idle.

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
	if (!fork()) {
		mark_n(50);
	} else {
		mark_n(50);
	}
}
