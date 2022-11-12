#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/sched.h>


#define MAX_SCHEDULED_ENVS 4*NENV

void sched_halt(void);
struct Env *get_env_to_run(void);

// Scheduling statistics
size_t calls_to_scheduler = 0;
envid_t scheduled_envs[MAX_SCHEDULED_ENVS]={0};
size_t env_executions[NENV] = {0};
size_t times_scheduled_envs = 0;


// 4 different queues, one for each priority
// (linked by Env->env_link)
struct env_queue env_priority_queues[NUMBER_OF_QUEUES] = {0};

int queues_runtime_threshold[] = { 256, 512, 1024 };

// Choose a user environment to run and run it.
struct Env *
pop_env_to_run(void)
{
	struct Env *to_run = NULL;
	// if using RoundRobin, there is only one queue
	for (int i = 0; i < NUMBER_OF_QUEUES; i++) {
		if (env_priority_queues[i].head != NULL) {
			to_run = env_priority_queues[i].head;
			to_run->priority = i;

			env_priority_queues[i].head = to_run->env_link;

			if (to_run->env_link == NULL)
				env_priority_queues[i].tail = NULL;
			return to_run;
		}
	}
	if (curenv && (curenv->env_status == ENV_RUNNABLE || curenv->env_status == ENV_RUNNING))
		return curenv;
	return NULL;
}

void
push_env_to_queue(struct Env *e)
{
	e->env_link = NULL;
	int queue_idx = e->priority;
	if (queue_idx < NUMBER_OF_QUEUES - 1 && e->vruntime > queues_runtime_threshold[queue_idx])
		queue_idx += 1;

	queue_idx = queue_idx < NUMBER_OF_QUEUES ? queue_idx : NUMBER_OF_QUEUES - 1;
	struct env_queue *queue = &env_priority_queues[queue_idx];

	if (queue->tail != NULL) {
		queue->tail->env_link = e;
	} else {
		queue->head = e;
	}
	queue->tail = e;
}

/*
 * Adds the env to the end of the list indicating order of env execution
*/

void add_env_to_metric(struct Env *to_run) {
	scheduled_envs[times_scheduled_envs] = to_run->env_id;
	times_scheduled_envs ++;
}

void print_statistics(){
	for(int i = 0; i < times_scheduled_envs; i++){
		env_executions[ENVX(scheduled_envs[i])]++;
		cprintf("Proccess executed:%d\n", scheduled_envs[i]);
		cprintf("Amount of executions: %d\n\n", env_executions[ENVX(scheduled_envs[i])]);
	}
}

/*
 * Find the next runnable env and run it.
 * This function never returns.
 */
void
sched_yield(void)
{
	calls_to_scheduler++;
	if (NENV == 0) {
		sched_halt();
	}
	struct Env *idle;
	struct Env *to_run = pop_env_to_run();

	if (to_run != NULL) {
		add_env_to_metric(to_run);
		env_run(to_run);
	}

	sched_halt();
}


void sched_halt(void) __attribute__((noreturn));

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here
	print_statistics();

	// Reset stack pointer, enable interrupts and then halt.

	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));

	for (;;)
		;
}
