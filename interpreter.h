#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "process.h"
#include "memory.h"

void execute_instruction(char* instruction, PCB* pcb, MemoryManager* mem);

#endif