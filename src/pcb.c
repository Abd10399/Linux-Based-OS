#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcb.h"

int pid_counter = 1;

int generatePID(){
    return pid_counter++;
}

//In this implementation, Pid is the same as file ID 
PCB* makePCB(int start, char* scriptName){
    PCB * newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->PC = start;
    newPCB->start  = start;
    newPCB->end = 0;
    newPCB->job_length_score = 1+0-start;
    newPCB->priority = false;
    newPCB->fcounter = 0;
    newPCB->lines_executed = 0;
    strcpy(newPCB->filename, scriptName);
    //initialize empty cells in array as page instances that are not initialized (initialized = false)
    for (int i = 0; i < 35; i++){
            newPCB->pagetbl[i] = makePAGE();
    }
    return newPCB;
}
