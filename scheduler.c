#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"
#include "process.h"
#include "memory.h"
#include "interpreter.h"
#include "queue.h"




// void fifo_scheduler(MemoryManager* memory, Queue* ready_queue) {
//     while (!is_empty(ready_queue)) {
//         PCB* current_process = dequeue(ready_queue);
//         printf("Executing Process ID: %d\n", current_process->pid);

//         while (current_process->program_counter < (current_process->mem_end) - 8) {
//             execute_instruction(memory, current_process);
//             //print_memory(memory);
//         }

//         printf("Process ID %d completed.\n", current_process->pid);
//     }
// }

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
    if (clock == arrival1) {
        printf("pcb1 has arrived\n");
        enqueue(&ready_queue, pcb1);
    }
    if (clock == arrival2) {
        printf("pcb2 has arrived\n");
        enqueue(&ready_queue, pcb2);
    }
    if (clock == arrival3) {
        printf("pcb2 has arrived\n");
        enqueue(&ready_queue, pcb3);
    }
}

void fifo_scheduler(MemoryManager* memory, Queue* ready_queue) {
    // Check for process arrivals
    printf("Clock: %d\n", clock);

    arrivals();
    // If no processes are ready, advance the clock
    if (is_empty(ready_queue)) {
        clock++;
        return;
    }  
    

    PCB* current_process = peek(ready_queue);

    if (current_process == NULL){
        clock ++;
        return;
    }

    

    // Check if the process has remaining instructions
    if (current_process->program_counter < current_process->mem_end - 8) {
        printf("Executing Process ID: %d\n", current_process->pid);
        execute_instruction(memory, current_process);
        
        if(current_process->program_counter >= current_process->mem_end - 8){
            programs--;
            printf("Process ID %d completed.\n", current_process->pid);
            dequeue(ready_queue);
        }
    } 
}

const int quanta = 2;
int current_quanta = quanta;

void round_robin(MemoryManager* mem , Queue* ready_queue){
    
    printf("Clock: %d\n", clock);


    arrivals();
    
    
    if(is_empty(ready_queue)){
        clock++;
        return;
    }
    

    if(current_quanta == 0){
        current_quanta = quanta;
        PCB* temp = dequeue(ready_queue);
        enqueue(ready_queue,temp);
    }

    PCB* current_process = peek(ready_queue);

    if(current_process->state == BLOCKED){
        current_quanta = quanta;
        current_process = check_blocked();
        if (current_process ==NULL){
            clock ++;
            return;
        }
    }

    if(current_process->program_counter < current_process->mem_end - 8){
        printf("Executing Process ID: %d\n", current_process->pid);
        execute_instruction(mem,current_process); // executing
        current_quanta--;
        if(current_process->program_counter >= current_process->mem_end - 8){
            current_quanta = quanta;
            programs--;
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
    if (clock == arrival1) {
        printf("Process ID %d has arrived.\n", pcb1->pid);
        enqueue(level1, pcb1);
        new_arrival = true;
    }
    if (clock == arrival2) {
        printf("Process ID %d has arrived.\n", pcb2->pid);
        enqueue(level1, pcb2);
        new_arrival = true;
    }
    if (clock == arrival3) {
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
            clock++; // Advance clock if no processes are ready
        }
    }else{
        clock ++;
    }
}

void execute_level(MemoryManager* mem, Queue* current_level, Queue* next_level, int quantum_index) {
    PCB* current_process = peek(current_level);
    printf("Executing Process ID: %d at Level with Quantum %d\n", current_process->pid, quantum[quantum_index]);

    execute_instruction(mem, current_process);
    cur_quantum[quantum_index]--;
    
    // Check if the process has completed execution
    if (current_process->program_counter < current_process->mem_end - 8) {
        if(cur_quantum[quantum_index] == 0) {
            current_queue = NULL;
            cur_quantum[quantum_index] = quantum[quantum_index];
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
        printf("Process ID %d completed.\n", current_process->pid);
        programs--;
        new_arrival = false;
    }
    
}