#include <inc/lib.h>

#define BLUE "\e[0;34m"
#define RESET "\e[0m"
#define RED "\e[0;31m"

void
umain(int argc, char **argv)
{
	int eid = sys_getenvid();

	cprintf("[setting niceness to 5]\n");
	sys_env_set_niceness(eid, 5);

	int i;
	cprintf("[forking]\n");
	if ((i = fork()) == 0) {
		cprintf(BLUE "child's niceness: %d\n" RESET,
		        sys_env_get_niceness(sys_getenvid()));
	} else {
		cprintf(BLUE "father's niceness: %d\n" RESET,
		        sys_env_get_niceness(sys_getenvid()));
	}
}
