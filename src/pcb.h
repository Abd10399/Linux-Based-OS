#ifndef PCB_H
#define PCB_H
#include <stdbool.h>
#include "paging.h"
/*
 * Struct:  PCB 
 * --------------------
 * pid: process(task) id
 * PC: program counter, stores the index of line that the task is executing
 * start: the first line in shell memory that belongs to this task
 * end: the last line in shell memory that belongs to this task
 * job_length_score: for EXEC AGING use only, stores the job length score
 */
typedef struct
{
    bool priority;
    //store filename so we can access it later for single page storing
    char filename[100];

    int pid;
    int PC;
    //start will contain first frame number
    int start;
    //end will contain total line
    int end;

    //keep track of the number of lines executed as we go
    int lines_executed;
    int job_length_score;

    //add page table
    //assume that each script doesnt exceed 100 lines and each frame is 3 lines
    //pagetbl contains the PAGE struct instances
    PAGE* pagetbl[35];

    //framecount that counts the numbers of lines executed in a page
    //used to determine when to go to the next frame
    int fcounter;
}PCB;

int generatePID();
PCB * makePCB(int start, char* scriptName);
#endif
