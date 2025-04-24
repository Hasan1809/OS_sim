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