#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "memory.h"
#include "queue.h"

extern int arrival1;
extern int arrival2;
extern PCB* pcb1;
extern PCB* pcb2;
extern int clock;
extern int programs;

void fifo_scheduler(MemoryManager* memory, Queue* ready_queue);
void round_robin(MemoryManager* mem , Queue* ready_queue, int rr);

#endif