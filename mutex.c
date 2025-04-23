// mutex.c
#include "mutex.h"
#include <stdio.h>

extern Queue readyQueue0, readyQueue1, readyQueue2; // You’ll manage priority queues here

void initMutex(Mutex* m) {
    m->locked = 0;
    initializeQueue(&m->waitingQ);
}

void semWait(MemoryManager* mem,Mutex* m, PCB* pcb) {
    if (m->locked == 0) {
        m->locked = 1;
    } else {
        // Block the process
        update_pcb_state_mem(mem,pcb,BLOCKED);
        enqueue(&m->waitingQ, pcb->pid); // or process pointer if you adjust the queue
        // Also add to global blocked queue if you have one
    }
}

void semSignal(Mutex *m) {
    if (isEmpty(&m->waitingQ)) {
        m->locked = 0;
    } else {
        // Unblock highest-priority process in the queue
        int pid = peek(&m->waitingQ); // You could store PCB* instead
        dequeue(&m->waitingQ);
        
        PCB *unblockedProcess = getPCBByPID(pid); // You’ll need to implement this helper
        unblockedProcess->state = READY;

        // Push into the appropriate ready queue
        if (unblockedProcess->priority == 0) enqueue(&readyQueue0, pid);
        else if (unblockedProcess->priority == 1) enqueue(&readyQueue1, pid);
        else enqueue(&readyQueue2, pid);
    }
}
