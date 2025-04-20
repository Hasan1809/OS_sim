#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"  // Include scheduler functions
#include "memory.h"     // Include memory functions
#include "process.h"    // Include process functions

int main(){

    PCB* pcb1 = create_pcb(1, 10);

    print_pcb(pcb1);





    return 0;
}

