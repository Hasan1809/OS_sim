// mutex.h
#ifndef MUTEX_H
#define MUTEX_H

#include "queue.h"
#include "process.h" 
#include "memory.h" 

typedef struct {
    int locked;       // 0 = unlocked, 1 = locked
    Queue waitingQ;   // Queue of blocked processes (you can use PCB* or int for PID)
} Mutex;

void initMutex(Mutex* m);
void semWait(MemoryManager*mem,Mutex* m, PCB* pcb);
void semSignal(Mutex* m);

#endif // MUTEX_H
