#ifndef _scheduler_h_
#define _scheduler_h_

#define CFS 0
#define SJF 1
#define PREEMPTIVE 1      // samo za SJF
#define NON_PREEMPTIVE 0  // samo za SJF


// load balancing promenljive
#define EMPTY 0
#define BALANCED 1
#define OVERLOADED 2
#define LOAD_BALANCING_THRESHOLD 100


typedef struct proc* (*getPtr)(int);
typedef void (*putPtr)(struct proc*, short);

struct GlobalScheduler
{
	getPtr get;
    putPtr put;
    struct spinlock mutex;
};


int set_scheduling_algorithm(int algorithm, short alpha, short algorithm_type);
void scheduler_init();

#endif