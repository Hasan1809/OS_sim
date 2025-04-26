// mutex.c
#include "mutex.h"
#include <stdio.h>


void initMutex(Mutex* m) {
    m->locked = 1;
    init_queue(&(m->waitingQ));
}

void semWait(MemoryManager* mem,Mutex* m, PCB* pcb) {
    if (m->locked == 1) {
        m->locked = 0;
    } else {
        // Block the process
        update_pcb_state_mem(mem,pcb,BLOCKED);
        print_queue(&ready_queue);
        enqueue(&(m->waitingQ), pcb); // or process pointer if you adjust the queue
        // Also add to global blocked queue if you have one
    }
}

void semSignal(MemoryManager* mem,Mutex *m) {
    if (is_empty(&m->waitingQ)) {
        m->locked = 1;
    } else {
        // Unblock highest-priority process in the queue
        PCB* pcb = peek(&(m->waitingQ)); // You could store PCB* instead
        dequeue(&(m->waitingQ));

        update_pcb_state_mem(mem,pcb,READY);

        // Push into the appropriate ready queue
        enqueue(&ready_queue, pcb);
        print_queue(&ready_queue);
    }
}
