#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"
#include "process.h"
#include "memory.h"
#include "interpreter.h"


void init_queue(Queue* q) {
    q->front = 0;
    q->rear = 0;
}

void enqueue(Queue* q, PCB* process) {
    q->processes[q->rear++] = process;
}

PCB* dequeue(Queue* q) {
    if (q->front == q->rear) {
        return NULL;
    }
    return q->processes[q->front++];
}

int is_empty(Queue* q) {
    return q->front == q->rear;
}

void fifo_scheduler(MemoryManager* memory, Queue* ready_queue) {
    while (!is_empty(ready_queue)) {
        PCB* current_process = dequeue(ready_queue);
        printf("Executing Process ID: %d\n", current_process->pid);

        while (current_process->program_counter < (current_process->mem_end) - 8) {
            execute_instruction(memory, current_process);
            //print_memory(memory);
        }

        printf("Process ID %d completed.\n", current_process->pid);
    }
}