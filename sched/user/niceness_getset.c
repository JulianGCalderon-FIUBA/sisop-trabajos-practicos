#include <inc/lib.h>

#define BLUE "\e[0;34m"
#define RESET "\e[0m"
#define RED "\e[0;31m"

void
umain(int argc, char **argv)
{
	int eid = sys_getenvid();

	cprintf("current niceness: %d\n", sys_env_get_niceness(eid));

	cprintf("[setting niceness to 5]\n");
	sys_env_set_niceness(eid, 5);

	cprintf("updated niceness: %d\n", sys_env_get_niceness(eid));


	if (sys_env_set_niceness(eid, -5) == -1) {
		cprintf("failed to update niceness\n");
	}
	int i;
	if ((i = fork() == 0)) {
		// HIJO MODIFICA EL PADRE; DEBERIA FALLAR
		if (sys_env_set_niceness(eid, 10) == -1)
			cprintf(RED "child: can't update father's "
			            "niceness\n" RESET);
	} else {
		// PADRE MODIFICA EL HIJO; DEBERIA FUNCIONAR
		sys_env_set_niceness(i, 10);
		cprintf(BLUE "father: can update child's niceness: %d\n" RESET,
		        sys_env_get_niceness(i));
	}
}
