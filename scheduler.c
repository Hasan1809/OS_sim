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

void fifo_scheduler(MemoryManager* memory, Queue* ready_queue) {
    // Check for process arrivals
    if (clock == arrival1) {
        printf("pcb1 has arrived\n");
        enqueue(ready_queue, pcb1);
    }
    if (clock == arrival2) {
        printf("pcb2 has arrived\n");
        enqueue(ready_queue, pcb2);
    }
    if (clock == arrival3) {
        printf("pcb2 has arrived\n");
        enqueue(ready_queue, pcb3);
    }

    // If no processes are ready, advance the clock
    if (is_empty(ready_queue)) {
        clock++;
        return;
    }

    printf("Clock: %d\n", clock);

    PCB* current_process = peek(ready_queue);

    // Check if the process has remaining instructions
    if (current_process->program_counter < current_process->mem_end - 8) {
        execute_instruction(memory, current_process);
        printf("Executing Process ID: %d\n", current_process->pid);
    } else {
        // Process is complete
        programs--;
        printf("Process ID %d completed.\n", current_process->pid);
        dequeue(ready_queue);

        // If there is another process, start executing it
        if (!is_empty(ready_queue)) {
            current_process = peek(ready_queue);
            execute_instruction(memory, current_process);
            printf("Executing Process ID: %d\n", current_process->pid);
        } else {
            clock++;
        }
    }
}

const int quanta = 2;
int current_quanta = quanta;

void round_robin(MemoryManager* mem , Queue* ready_queue){
    if(clock == arrival1){
        enqueue(ready_queue,pcb1);
    }if(clock == arrival2){
        enqueue(ready_queue,pcb2);
    }   
    
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

    if(current_process->program_counter < current_process->mem_end - 8){
        execute_instruction(mem,current_process); // executing
        printf("Executing Process ID: %d\n", current_process->pid);
        current_quanta--;
    }else{ //if its over
        programs--;
        printf("Process ID %d completed.\n", current_process->pid);
        dequeue(ready_queue);
        if(is_empty(ready_queue)){
            clock++;
            return;
        } 
        current_process = peek(ready_queue);
        current_quanta = quanta;
        current_quanta--;
        execute_instruction(mem,current_process);
        printf("Executing Process ID: %d\n", current_process->pid);
    }
}

const int quantum[] = {1, 2, 4, 8}; // Quantum for each level
int cur_quantum[] = {1,2,4,8};

void multilevel_feedback_queue(MemoryManager* mem, Queue* level1, Queue* level2, Queue* level3, Queue* level4) {
    

    if (!is_empty(level1) || !is_empty(level2) || !is_empty(level3) || !is_empty(level4) || programs > 0) {
        // Check for process arrivals
        if (clock == arrival1) {
            printf("Process ID %d has arrived.\n", pcb1->pid);
            enqueue(level1, pcb1);
        }
        if (clock == arrival2) {
            printf("Process ID %d has arrived.\n", pcb2->pid);
            enqueue(level1, pcb2);
        }
        if (clock == arrival3) {
            printf("Process ID %d has arrived.\n", pcb3->pid);
            enqueue(level1, pcb3);
        }

        // Process each level in order of priority
        if (!is_empty(level1)) {
            execute_level(mem, level1, level2, 0);
        } else if (!is_empty(level2)) {
            execute_level(mem, level2, level3, 1);
        } else if (!is_empty(level3)) {
            execute_level(mem, level3, level4, 2);
        } else if (!is_empty(level4)) {
            execute_level(mem, level4, level4, 3); // Round Robin at level 4
        } else {
            clock++; // Advance clock if no processes are ready
        }
    }
}

void execute_level(MemoryManager* mem, Queue* current_level, Queue* next_level, int quantum_index) {
    PCB* current_process = peek(current_level);
    printf("Executing Process ID: %d at Level with Quantum %d\n", current_process->pid, quantum[quantum_index]);

    execute_instruction(mem, current_process);
    cur_quantum[quantum_index]--;
    

    // Check if the process has completed execution
    if (current_process->program_counter < current_process->mem_end - 8) {
        if(cur_quantum[quantum_index]==0){
            cur_quantum[quantum_index] = quantum[quantum_index];
            dequeue(current_level);
        if (current_level != next_level) {
            printf("Process ID %d moved to the next level.\n", current_process->pid);
            enqueue(next_level, current_process); // Move to the next level
        } else {
            enqueue(current_level, current_process); // Re-enqueue in the same level (Round Robin)
        }}
    } else {
        dequeue(current_level);
        cur_quantum[quantum_index] = quantum[quantum_index];
        printf("Process ID %d completed.\n", current_process->pid);
        programs--; // Decrease the count of active programs
    }
}