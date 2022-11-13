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


void process1(void);
void process2(void);
void process3(void);

void mark(char *message);
void mark_n(char *message, int n);
int mark_fork(char *message);

void
mark(char *message)
{
	if (message)
		cprintf("\033[0;31m %s \033[0;37m \n", message);
	sys_yield();
}


void
mark_n(char *message, int n)
{
	for (int i = 0; i < n; i++) {
		mark(message);
	}
}

int
mark_fork(char *message)
{
	if (message)
		cprintf("%s\n", message);

	return fork();
}


void
process1()
{
	if (!mark_fork("P1: forking p2")) {
		process2();
	} else {
		mark_n("P1: yield", 3);

		mark_n("P1: yield2", 5);
	}
}

void
process2()
{
	mark_n("P2: yield", 3);
	if (!mark_fork("P2: forking p3")) {
		process3();
	} else {
		mark_n("P2: yield2", 5);
	}
}

void
process3()
{
	mark_n("P3: yield", 4);
	mark_n("P3: yield2", 5);
}

void
queue_push_test()
{
	if (mark_fork("forking")) {
		mark_n("1yield", 20);
	} else {
		mark_n("2yield", 19);
	}
}


void
umain(int argc, char **argv)
{
	// queue_push_test();
	process1();
}
