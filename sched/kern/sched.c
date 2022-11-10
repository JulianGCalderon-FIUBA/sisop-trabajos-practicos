#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/sched.h>

void sched_halt(void);
struct Env *get_env_to_run(void);

// Scheduling statistics
size_t calls_to_scheduler = 0;
struct executed_envs *scheduled_envs = NULL;

#undef ROUNDROBIN
#ifndef PRIORITY
#define ROUNDROBIN
#endif  // !PRIORITY

// 4 different queues, one for each priority
// (linked by Env->env_link)
struct env_queue env_priority_queues[NUMBER_OF_QUEUES];

int queues_runtime_threshold[] = { 256, 512, 1024 };

// Choose a user environment to run and run it.
struct Env *
pop_env_to_run(void)
{
	struct Env *to_run = NULL;
	for (int i = 0; i < 4; i++) {
		if (env_priority_queues[i].head != NULL) {
			to_run = env_priority_queues[i].head;
			to_run->priority = i;

			env_priority_queues[i].head = to_run->env_link;

			if (to_run->env_link == NULL)
				env_priority_queues[i].tail = NULL;

			return to_run;
		}
	}

	return NULL;
}

void
push_env_to_queue(struct Env *e)
{
	int queue_idx = e->priority;
	if (queue_idx < NUMBER_OF_QUEUES - 1 &&
	    e->vruntime > queues_runtime_threshold[queue_idx])
		queue_idx += 1;

	queue_idx = queue_idx < NUMBER_OF_QUEUES ? queue_idx : NUMBER_OF_QUEUES;
	struct env_queue *queue = &env_priority_queues[queue_idx];

	if (queue->tail != NULL) {
		queue->tail->env_link = e;
	} else {
		queue->head = e;
	}
	queue->tail = e;
}

void
sched_yield(void)
{
	calls_to_scheduler++;
	if (NENV == 0) {
		sched_halt();
	}
	struct Env *idle;

#ifdef ROUNDROBIN

	int curenv_pos = curenv ? ENVX(curenv->env_id) : 0;
	int i = (curenv_pos + 1) % NENV;

	while (envs[i].env_status != ENV_RUNNABLE && i != curenv_pos) {
		i++;

		if (i == NENV) {
			i = 0;
		}
	}

	if (curenv && envs[i].env_status == ENV_RUNNING)
		env_run(&envs[i]);

	if (envs[i].env_status == ENV_RUNNABLE)
		env_run(&envs[i]);

#endif  // ROUNDROBIN

#ifdef PRIORITY

	struct Env *to_run = pop_env_to_run();

	if (to_run != NULL) {
		struct executed_envs executed;
		if (scheduled_envs == NULL) {
			executed.last_executed->env = to_run;
			scheduled_envs = executed;
		} else {
			scheduled_envs->last_executed->next = executed;
			scheduled_envs->last_executed = executed;
		}
		executed.env = to_run;
		executed.next = NULL;

		env_run(to_run);
	}
#endif  // PRIORITY

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

#ifdef PRIORITY

	cprintf("SCHEDULING STATISTICS\n");
	cprintf("Calls to scheduler: %d\n", calls_to_scheduler);
	while (scheduled_envs->env) {
		cprintf("Executed env: %d\n", scheduled_envs->env->env_id);
		cprintf("Number of times env was executed: %d\n",
		        scheduled_envs->env->env_runs);
		scheduled_envs = scheduled_envs->next;
	}

#endif

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
