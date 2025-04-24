// C Program to demonstrate how to Implement a queue
#include <stdbool.h>
#include <stdio.h>
#include "queue.h"
#include "process.h"
#define MAX_SIZE 100

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

PCB* peek(Queue* q) {
    if (q->front == q->rear) {
        return NULL;
    }
    return q->processes[q->front];
}

int is_empty(Queue* q) {
    return q->front == q->rear;
}


