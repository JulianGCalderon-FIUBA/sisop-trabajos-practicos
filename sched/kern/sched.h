/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_SCHED_H
#define JOS_KERN_SCHED_H
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

struct env_queue {
	struct Env *head;
	struct Env *tail;
};

#define DEFAULT_NICENESS 1
#define BEST_NICENESS -19
#define WORST_NICENESS 20

// Set NUMBER_OF_QUEUES to 1 for Round-Robin
#define NUMBER_OF_QUEUES 4
extern struct env_queue env_priority_queues[NUMBER_OF_QUEUES];
int queues_runtime_threshold[NUMBER_OF_QUEUES - 1] = { 8, 12, 16 };

// This function does not return.
void sched_yield(void) __attribute__((noreturn));

void push_env_to_queue(struct Env *e);

#endif  // !JOS_KERN_SCHED_H
