#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/sched.h>

#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
// #include <kern/sched.h>


#define MAX_SCHEDULED_ENVS 4 * NENV

// the last queue doesn't have a threshold, since envs in said queue can't go any lower
int queues_vruntime_threshold[NUMBER_OF_QUEUES - 1] = { 64, 256, 1024 };
// the thresholds should be defined according to the values in
// niceness_to_vruntime_weight (kern/env.c) vruntime is set to 0 every time an
// env changes queue (during boosting or due to env reaching threshold), must be
// taken into account to set the values in queues_vruntime_threshold

uint32_t envs_ran_since_boost = 0;

static const int niceness_to_vruntime_weight[] = {
	1,   1,    1,    2,    2,    3,    4,    5,    6,    8,
	10,  13,   16,   20,   26,   32,   40,   51,   64,   80,
	100, 124,  156,  194,  242,  305,  376,  476,  595,  747,
	930, 1177, 1462, 1828, 2275, 2844, 3531, 4452, 5688, 6826
};

int
get_vruntime_weight_for_niceness(int niceness)
{
	return niceness_to_vruntime_weight[niceness + 19];
}

void sched_halt(void);

// Scheduling statistics
size_t calls_to_scheduler = 0;
envid_t scheduled_envs[MAX_SCHEDULED_ENVS] = { 0 };
size_t env_executions[NENV] = { 0 };
size_t times_scheduled_envs = 0;

// 4 different queues, one for each priority
// (linked by Env->env_link)
struct env_queue env_priority_queues[NUMBER_OF_QUEUES] = { 0 };

// Each env remembers the value of calls_to_sched_boosting during its last
// execution. If it differs from the current calls_to_sched_boosting, it means
// that a boosting took place. If a boosting took place, the amount of times the
// env was executed should be forgotten (set to 0)
uint32_t calls_to_sched_boosting = 0;

/*
 *  ################ QUEUES ################
 */

// Returns first runnable env in queues.
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

			// cprintf("[popping %d from %d]\n", to_run->env_id, i);

			return to_run;
		}
	}

	return NULL;
}


// Push env to corresponding queue.
// If env has run more than the threshold for that queue it gets demoted to lower queue.
void
push_env_to_queue(struct Env *e)
{
	e->env_link = NULL;
	if (e->last_sched_boost_known < calls_to_sched_boosting) {
		// a boosting took place, reset the env's vruntime and start from scratch
		e->priority = 0;
		e->last_sched_boost_known = calls_to_sched_boosting;
		// it was run 1 time since boost, and vruntime = weight *
		// times_ran = vruntime_coeficient_for_niceness * 1
		curenv->vruntime = get_vruntime_weight_for_niceness(e->niceness);
	}

	int queue_idx = e->priority;
	// if vruntime is higher than threshold, move env to the next queue
	if (queue_idx < NUMBER_OF_QUEUES - 1 &&
	    e->vruntime > queues_vruntime_threshold[queue_idx]) {
		e->vruntime = 0;
		queue_idx += 1;
	}

	struct env_queue *queue = &env_priority_queues[queue_idx];
	if (queue->tail != NULL) {
		queue->tail->env_link = e;
	} else {
		queue->head = e;
	}
	queue->tail = e;

	// cprintf("[pushing %d to %d]\n", e->env_id, queue_idx);
}

// Boost all envs to first queue.
void
sched_boosting(void)
{
	++calls_to_sched_boosting;
	struct Env *first_env = NULL;
	struct Env *last_env = NULL;

	// join all queues into one large queue (the first one)
	for (int i = 0; i < NUMBER_OF_QUEUES - 1; i++) {
		if (first_env == NULL) {
			first_env = env_priority_queues[i].head;
			last_env = env_priority_queues[i].tail;
		}

		// append next queue to the end of current queue
		if (last_env && env_priority_queues[i + 1].head) {
			last_env->env_link = env_priority_queues[i + 1].head;
			last_env = env_priority_queues[i + 1].tail;
		}

		// mark all queues as empty
		env_priority_queues[i].head = NULL;
		env_priority_queues[i].tail = NULL;
	}

	if (last_env == NULL) {
		first_env = env_priority_queues[NUMBER_OF_QUEUES - 1].head;
		last_env = env_priority_queues[NUMBER_OF_QUEUES - 1].tail;
	}

	// mark the last queue as empty
	env_priority_queues[NUMBER_OF_QUEUES - 1].head = NULL;
	env_priority_queues[NUMBER_OF_QUEUES - 1].tail = NULL;

	env_priority_queues[0].head = first_env;
	env_priority_queues[0].tail = last_env;

#define RESET "\e[0m"
#define RED "\e[0;31m"

	// cprintf(RED "[boosting]\n" RESET);
}

/*
 *  ################ METRICS ################
 */

/*
 * Adds the env to the end of the list indicating order of env execution
 */
void
add_env_to_metric(struct Env *to_run)
{
#ifdef STATS
	scheduled_envs[times_scheduled_envs] = to_run->env_id;
	times_scheduled_envs++;
#endif
}

void
print_statistics()
{
#ifdef STATS
	for (int i = 0; i < times_scheduled_envs; i++) {
		env_executions[ENVX(scheduled_envs[i])]++;
		cprintf("Proccess executed:%d\n", scheduled_envs[i]);
		cprintf("Amount of executions: %d\n\n",
		        env_executions[ENVX(scheduled_envs[i])]);
	}
#endif
}

/*
 *  ################ SCHEDULING ################
 */

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

	if (curenv && curenv->env_status == ENV_RUNNING) {
		curenv->env_status = ENV_RUNNABLE;
		push_env_to_queue(curenv);
	}

	if (envs_ran_since_boost >= RUNS_PER_BOOST) {
		sched_boosting();
		envs_ran_since_boost = 0;
	}

	struct Env *runnable_env = pop_env_to_run();

	if (runnable_env == NULL)
		sched_halt();

	runnable_env->vruntime +=
	        get_vruntime_weight_for_niceness(runnable_env->niceness);

	// add_env_to_metric(runnable_env);
	++envs_ran_since_boost;
	// cprintf("[running %d]\n", runnable_env->env_id);
	env_run(runnable_env);
}

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
}
