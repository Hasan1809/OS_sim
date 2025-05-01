#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"
#include "process.h"
#include "memory.h"
#include "interpreter.h"
#include "queue.h"




static PCB* check_blocked(){
    while(peek(&ready_queue)->state == BLOCKED){
        dequeue(&ready_queue);
        if(is_empty(&ready_queue)){
            return NULL;
        }
    }
    return peek(&ready_queue);
}

static void arrivals(){
    if (os_clock == arrival1) {
        printf("pcb1 has arrived\n");
        enqueue(&ready_queue, pcb1);
    }
    if (os_clock == arrival2) {
        printf("pcb2 has arrived\n");
        enqueue(&ready_queue, pcb2);
    }
    if (os_clock == arrival3) {
        printf("pcb2 has arrived\n");
        enqueue(&ready_queue, pcb3);
    }
}

void fifo_scheduler(MemoryManager* memory, Queue* ready_queue) {
    // Check for process arrivals
    printf("os_clock: %d\n", os_clock);

    arrivals();
    // If no processes are ready, advance the os_clock
    if (is_empty(ready_queue)) {
        os_clock++;
        return;
    }  
    

    PCB* current_process = peek(ready_queue);

    if(current_process->state != RUNNING){
        update_pcb_state_mem(memory,current_process,RUNNING);
    }

    if (current_process == NULL){
        os_clock ++;
        return;
    }

    

    // Check if the process has remaining instructions
    if (current_process->program_counter < current_process->mem_end - 8) {
        printf("Executing Process ID: %d\n", current_process->pid);
        execute_instruction(memory, current_process);
        
        if(current_process->program_counter >= current_process->mem_end - 8){
            programs--;
            printf("Process ID %d completed.\n", current_process->pid);
            update_pcb_state_mem(memory,current_process,TERMINATED);
            dequeue(ready_queue);
        }
    } 
}


void init_quanta(){
    current_quanta = quanta;
}

void round_robin(MemoryManager* mem , Queue* ready_queue){
    
    printf("os_clock: %d\n", os_clock);


    arrivals();
    
    
    if(is_empty(ready_queue)){
        os_clock++;
        return;
    }
    

    if(current_quanta == 0){
        current_quanta = quanta;
        PCB* temp = dequeue(ready_queue);
        if(temp->state != BLOCKED){
            update_pcb_state_mem(mem,temp,READY);
        }
        enqueue(ready_queue,temp);
    }

    PCB* current_process = peek(ready_queue);
    

    if(current_process->state == BLOCKED){
        current_quanta = quanta;
        current_process = check_blocked();
        if (current_process ==NULL){
            os_clock ++;
            return;
        }
    }

    
    
    

    if(current_process->program_counter < current_process->mem_end - 8){
        printf("Executing Process ID: %d\n", current_process->pid);
        update_pcb_state_mem(mem,current_process,RUNNING);
        execute_instruction(mem,current_process); // executing
        current_quanta--;
        if(current_process->program_counter >= current_process->mem_end - 8){
            current_quanta = quanta;
            programs--;
            update_pcb_state_mem(mem,current_process,TERMINATED);
            printf("Process ID %d completed.\n", current_process->pid);
            dequeue(ready_queue);
        }
    }
}

const int quantum[] = {1, 2, 4, 8}; // Quantum for each level
int cur_quantum[] = {1, 2, 4, 8};
bool new_arrival = false;
Queue* current_queue = NULL;  // Initialize to NULL at global scope
Queue* next_queue = NULL;

void multilevel_feedback_queue(MemoryManager* mem, Queue* level1, Queue* level2, Queue* level3, Queue* level4) {
    // Check for process arrivals first
    if (os_clock == arrival1) {
        printf("Process ID %d has arrived.\n", pcb1->pid);
        enqueue(level1, pcb1);
        new_arrival = true;
    }
    if (os_clock == arrival2) {
        printf("Process ID %d has arrived.\n", pcb2->pid);
        enqueue(level1, pcb2);
        new_arrival = true;
    }
    if (os_clock == arrival3) {
        printf("Process ID %d has arrived.\n", pcb3->pid);
        enqueue(level1, pcb3);
        new_arrival = true;
    }

    if (!is_empty(level1) || !is_empty(level2) || !is_empty(level3) || !is_empty(level4) || programs > 0) {
        
        // Process each level in order of priority
        if (!is_empty(level1)) {
            if (new_arrival && (current_queue != level1 && current_queue != NULL)) {
                // Finish current quantum before potentially switching
                if (current_queue == level2) {
                    execute_level(mem, level2, level3, 1);
                } else if (current_queue == level3) {
                    execute_level(mem, level3, level4, 2);
                } else if (current_queue == level4) {
                    execute_level(mem, level4, level4, 3);
                }
            } else {
                current_queue = level1; // Set current queue before executing
                next_queue = level2;
                execute_level(mem, level1, level2, 0);
            }
        } else if (!is_empty(level2)) {
            current_queue = level2;
            execute_level(mem, level2, level3, 1);
        } else if (!is_empty(level3)) {
            current_queue = level3;
            execute_level(mem, level3, level4, 2);
        } else if (!is_empty(level4)) {
            current_queue = level4;
            execute_level(mem, level4, level4, 3);
        } else {
            current_queue = NULL; // No queue active
            os_clock++; // Advance os_clock if no processes are ready
        }
    }
}

void execute_level(MemoryManager* mem, Queue* current_level, Queue* next_level, int quantum_index) {
    PCB* current_process = peek(current_level);
    printf("Executing Process ID: %d at Level with Quantum %d\n", current_process->pid, quantum[quantum_index]);
    if (current_process->state != RUNNING){
        update_pcb_state_mem(mem, current_process, RUNNING);
    }
    execute_instruction(mem, current_process);
    if(current_process->state==BLOCKED){
        dequeue(current_level);
        current_queue = NULL;
        cur_quantum[quantum_index] = quantum[quantum_index];
        return;
    }
    cur_quantum[quantum_index]--;
    
    // Check if the process has completed execution
    if (current_process->program_counter < current_process->mem_end - 8) {
        if(cur_quantum[quantum_index] == 0) {
            current_queue = NULL;
            cur_quantum[quantum_index] = quantum[quantum_index];
            update_pcb_state_mem(mem, current_process, READY);
            dequeue(current_level);
            if (current_level != next_level) {
                printf("Process ID %d moved to the next level.\n", current_process->pid);
                enqueue(next_level, current_process);
            } else {
                enqueue(current_level, current_process);
            }
            new_arrival = false;
        }
    } else {
        dequeue(current_level);
        cur_quantum[quantum_index] = quantum[quantum_index];
        update_pcb_state_mem(mem, current_process, TERMINATED);
        printf("Process ID %d completed.\n", current_process->pid);
        programs--;
        new_arrival = false;
        current_queue = NULL;
    }
    print_queue(&lvl1);
}