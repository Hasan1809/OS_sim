#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"  // Include scheduler functions
#include "memory.h"     // Include memory functions
#include "process.h" 
#include "interpreter.h"   // Include process functions
#include "queue.h"
#include "mutex.h"



char** separatefunction(char* fileName, int* line_count) {
    FILE *file = fopen(fileName, "r");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    char ch;
    char result[100] = "";
    char** lines = malloc(sizeof(char*) * 100); // support up to 100 lines
    int i = 0;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            lines[i++] = strdup(result); // save the current line
            result[0] = '\0';            // clear buffer
        } else {
            char temp[2] = {ch, '\0'};
            strcat(result, temp);        // build line char by char
        }
    }

    // Handle last line if file doesn’t end with newline
    if (strlen(result) > 0) {
        lines[i++] = strdup(result);
    }

    fclose(file);
    *line_count = i;
    return lines;
}

int arrival1;
int arrival2;
int arrival3;
PCB* pcb1;
PCB* pcb2;
PCB* pcb3;
int clock = 0;
int programs = 3;
Queue lvl1;
Queue lvl2;
Queue lvl3;
Queue lvl4;
Queue ready_queue;
Mutex file;
Mutex input;
Mutex output;

int main(){

    init_queue(&ready_queue);
    init_queue(&lvl1);
    init_queue(&lvl2);
    init_queue(&lvl3);
    init_queue(&lvl4);
    initMutex(&file);
    initMutex(&input);
    initMutex(&output);

    MemoryManager mem[60];
    init_memory(mem);

    arrival1 = 0;
    arrival2 = 0;
    arrival3 = 9;
    
    pcb1 = create_pcb(1, 10);
    pcb2 = create_pcb(2,12);
    pcb3 = create_pcb(3,14);


    int line_count;
    char** lines = separatefunction("Program_1.txt", &line_count);
    int line_count2;
    char** lines2 = separatefunction("Program_2.txt", &line_count2);
    int line_count3;
    char** lines3 = separatefunction("Program_3.txt", &line_count3);
    
    allocate_process(mem,pcb1,lines,line_count);   
    allocate_process(mem,pcb2,lines2,line_count2);
    allocate_process(mem,pcb3,lines3,line_count3);

    
    print_memory(mem);

    while(programs>0){
        multilevel_feedback_queue(mem, &lvl1, &lvl2,&lvl3,&lvl4);
    }

    // while(programs>0){
    //     round_robin(mem,&ready_queue);
    // }

    // while(programs>0){
    //     fifo_scheduler(mem, &ready_queue);
    // }

    //fifo_scheduler(mem, &ready_queue);

    print_memory(mem);


       
    





    return 0;
}

