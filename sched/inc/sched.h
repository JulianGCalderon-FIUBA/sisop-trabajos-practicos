#ifndef JOS_INC_SCHED_H
#define JOS_INC_SCHED_H

#define DEFAULT_NICENESS 1
#define BEST_NICENESS -19
#define WORST_NICENESS 20

// minimum amount of env runs that must be performed in-between boostings
#define MIN_RUNS_PER_BOOST 64

// Set NUMBER_OF_QUEUES to 1 for Round-Robin
#define NUMBER_OF_QUEUES 4

struct env_queue {
	struct Env *head;
	struct Env *tail;
};

extern struct env_queue env_priority_queues[NUMBER_OF_QUEUES];

void push_env_to_queue(struct Env *e);

#endif // !JOS_INC_SCHED_H
