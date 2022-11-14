#ifndef JOS_INC_SCHED_H
#define JOS_INC_SCHED_H

#define DEFAULT_NICENESS 1
#define BEST_NICENESS -19
#define WORST_NICENESS 20

// amount of env runs that must be performed in-between boostings
#define RUNS_PER_BOOST 64

// Set NUMBER_OF_QUEUES to 1 for Round-Robin
// NUMBER_OF_QUEUES should be a number between 1 and MAX_QUEUES
#ifdef ROUND_ROBIN
#define NUMBER_OF_QUEUES 1
#else
#define NUMBER_OF_QUEUES 4
#endif

#define MAX_QUEUES 4
struct env_queue {
	struct Env *head;
	struct Env *tail;
};

extern struct env_queue env_priority_queues[NUMBER_OF_QUEUES];

void push_env_to_queue(struct Env *e);

#endif  // !JOS_INC_SCHED_H
