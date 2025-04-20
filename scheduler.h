#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

#define MAX_PROCESSES 10

typedef struct {
    PCB* ready_queue[MAX_PROCESSES];
    int count;
} Scheduler;

// Scheduler functions
void init_scheduler(Scheduler* sched);
void add_process(Scheduler* sched, PCB* pcb);
PCB* get_next_process(Scheduler* sched);

#endif