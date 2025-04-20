#include "memory.h"
#include <string.h>
#include <stdio.h>

void init_memory(MemoryManager* mem) {
    for (int i = 0; i < MEM_SIZE; i++) {
        mem->words[i].key = NULL;
        mem->words[i].value = NULL;
        mem->used[i] = 0;
    }
}

int allocate_process(MemoryManager* mem, PCB* pcb, char** program, int program_len) {
    // Calculate needed space: program + vars + PCB fields (6 words)
    int needed = program_len + MAX_VARIABLES + 6;
    
    // Find contiguous block
    int start = -1, consecutive = 0;
    for (int i = 0; i < MEM_SIZE; i++) {
        if (!mem->used[i]) {
            if (start == -1) start = i;
            if (++consecutive >= needed) break;
        } else {
            start = -1;
            consecutive = 0;
        }
    }
    
    if (consecutive < needed) return -1; // No space
    
    // Allocate program instructions
    for (int i = 0; i < program_len; i++) {
        mem->words[start + i].key = "instruction";
        mem->words[start + i].value = strdup(program[i]);
        mem->used[start + i] = 1;
    }
    
    // Allocate variables
    for (int i = 0; i < MAX_VARIABLES; i++) {
        mem->words[start + program_len + i].key = "variable";
        mem->words[start + program_len + i].value = NULL;
        mem->used[start + program_len + i] = 1;
    }
    
    // Store PCB fields
    pcb->mem_start = start;
    pcb->mem_end = start + needed - 1;
    
    mem->words[pcb->mem_end - 5].key = "pid";
    mem->words[pcb->mem_end - 5].value = strdup("1"); // Actual PID set later
    
    // ... (store other PCB fields similarly)
    
    return start;
}