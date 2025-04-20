#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"  // Include scheduler functions
#include "memory.h"     // Include memory functions
#include "process.h"    // Include process functions

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


int main(){

    PCB* pcb1 = create_pcb(1, 10);

    print_pcb(pcb1);

    int line_count;
    char** lines = separatefunction("Program_1.txt", &line_count);

    MemoryManager mem[60];
    init_memory(mem);
    allocate_process(mem,pcb1,lines,line_count);
    print_memory(mem);

    
    

       
    





    return 0;
}

