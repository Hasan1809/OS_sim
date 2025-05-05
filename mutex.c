// mutex.c
#include "mutex.h"
#include <stdio.h>


void initMutex(Mutex* m) {
    m->pid=-1;
    m->locked = 0;
    init_queue(&(m->waitingQ));
}

void semWait(MemoryManager* mem,Mutex* m, PCB* pcb) {
    if (m->locked == 0) {
        m->pid = pcb->pid;
        m->locked = 1;
    } else {
        // Block the process
        update_pcb_state_mem(mem,pcb,BLOCKED);

        printf("process: %d is blocked \n", pcb->pid);
        
        enqueue(&(m->waitingQ), pcb); // or process pointer if you adjust the queue
        // Also add to global blocked queue if you have one
    }
}

void semSignal(MemoryManager* mem,Mutex *m,PCB* pcb) {
    if(m->pid == pcb->pid){
        if (is_empty(&m->waitingQ)) {
            m->locked = 0;
        } else {
        // Unblock highest-priority process in the queue
            PCB* pcb = peek(&(m->waitingQ)); // You could store PCB* instead
            dequeue(&(m->waitingQ));
            m->pid = pcb->pid;
            update_pcb_state_mem(mem,pcb,READY);

        // Push into the appropriate ready queue
        if (schedule != MLFQ){
            enqueue(&ready_queue, pcb);
        }
        }
    }
}
