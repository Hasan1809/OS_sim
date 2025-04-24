#include "interpreter.h"
#include "memory.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Helper: Get variable value from memory
static char* get_var_value(MemoryManager* mem, PCB* pcb, const char* var_name) {
    for (int i = pcb->mem_start; i <= pcb->mem_end; i++) {
        if (mem->words[i].key && strcmp(mem->words[i].key, "variable") == 0 && 
            mem->words[i].value && strstr(mem->words[i].value, var_name)) {
            return strchr(mem->words[i].value, '=') + 1;
        }
    }
    return NULL;
}

// Helper: Set variable in memory
static void set_var_value(MemoryManager* mem, PCB* pcb, const char* var_name, const char* value) {
    for (int i = pcb->mem_start; i <= pcb->mem_end; i++) {
        if (mem->words[i].key && strcmp(mem->words[i].key, "variable") == 0) {
            if (!mem->words[i].value || strstr(mem->words[i].value, var_name)) {
                free(mem->words[i].value);
                mem->words[i].value = malloc(strlen(var_name) + strlen(value) + 2);
                sprintf(mem->words[i].value, "%s=%s", var_name, value);
                return;
            }
        }
    }
    fprintf(stderr, "Error: No variable slot found for %s\n", var_name);
}

char* get_current_instruction(MemoryManager* mem, PCB* pcb){
    return mem->words[pcb->program_counter].value;
}

void execute_instruction (MemoryManager* mem ,PCB* pcb ){
    char* instruction = get_current_instruction(mem,pcb);
    char cmd[20], arg1[20], arg2[20];
    sscanf(instruction, "%s %s %s", cmd, arg1, arg2);

    if (strcmp(cmd, "print") == 0) {
        // Handle print command (requires userOutput mutex)
        char* val = get_var_value(mem, pcb, arg1);
        printf("%s\n", val ? val : arg1); // Print variable or literal
    }
    else if (strcmp(cmd, "assign") == 0) {
        // Handle variable assignment
        if (strcmp(arg2, "input") == 0) {
            printf("Please enter a value: ");
            char input[50];
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = '\0'; // Remove newline
            set_var_value(mem, pcb, arg1, input);
        } else {
            set_var_value(mem, pcb, arg1, arg2);
        }
    }
    else if (strcmp(cmd, "printFromTo") == 0) {
        // Handle number range printing
        int from = atoi(get_var_value(mem, pcb, arg1) ?: arg1);
        int to = atoi(get_var_value(mem, pcb, arg2) ?: arg2);
        for (int i = from; i <= to; i++) {
            printf("%d\n", i);
        }
    }
    else if (strcmp(cmd, "writeFile") == 0) {
        // Handle file writing
        char* filename = arg1;
        char* data = arg2;
        FILE* file = fopen(filename, "r+");
        if (file) {
            fprintf(file, "%s", data);
            fclose(file);
        } else {
            fprintf(stderr, "Error writing to file %s\n", filename);
        }
    }
    else if (strcmp(cmd, "readFile") == 0) {
        // Handle file reading
        char* filename = get_var_value(mem, pcb, arg1) ?: arg1;
        FILE* file = fopen(filename, "r");
        if (file) {
            char buffer[100];
            fgets(buffer, sizeof(buffer), file);
            fclose(file);
            set_var_value(mem, pcb, "file_content", buffer);
        } else {
            fprintf(stderr, "Error reading file %s\n", filename);
            set_var_value(mem, pcb, "file_content", "FILE_NOT_FOUND");
        }
    }
    else if (strcmp(cmd, "semWait") == 0) {
        // Handle mutex acquisition (stub - integrate with your mutex system)
        printf("Process %d: Waiting for %s\n", pcb->pid, arg1);
        update_pcb_state_mem(mem, pcb , BLOCKED);
    }
    else if (strcmp(cmd, "semSignal") == 0) {
        // Handle mutex release (stub)
        printf("Process %d: Releasing %s\n", pcb->pid, arg1);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
    }
    increment_program_counter_mem(mem,pcb);
    clock ++;
}