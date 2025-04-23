#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "memory.h"


typedef struct Queue {
    PCB* processes[100];
    int front;
    int rear;
} Queue;

void init_queue(Queue* q);
void enqueue(Queue* q, PCB* process);
PCB* dequeue(Queue* q);
int is_empty(Queue* q);
void fifo_scheduler(MemoryManager* memory, Queue* ready_queue);

#endif