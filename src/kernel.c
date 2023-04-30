#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "pcb.h"
#include "kernel.h"
#include "shell.h"
#include "shellmemory.h"
#include "interpreter.h"
#include "ready_queue.h"

bool multi_threading = false;
pthread_t worker1;
pthread_t worker2;
bool active = false;
bool debug = false;
bool in_background = false;
pthread_mutex_t queue_lock;

int scriptNum = 0;



void lock_queue(){
    if(multi_threading) pthread_mutex_lock(&queue_lock);
}

void unlock_queue(){
    if(multi_threading) pthread_mutex_unlock(&queue_lock);
}

void replace_semicolons(char *buffer) {
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == ';') {
            buffer[i] = '\n';
        }
    }
}

//NEED TO CHANGE FOR 1.2.2 TO INITIALIZE BY STORING 2 PAGES ONLY
int process_initialize(char *filename){
    FILE* fp;
    int error_code = 0;
    int* start = (int*)malloc(sizeof(int));
    int* end = (int*)malloc(sizeof(int));
    
    fp = fopen(filename, "rt");
    if(fp == NULL){
        error_code = 11; // 11 is the error code for file does not exist
        return error_code;
    }

    //copy the file to backingstore before loading
	char* command = (char*) calloc(1, 100); 
	strncat(command, "cp ", 5);
	strncat(command, filename, 1+strlen(filename));
	strncat(command, " backingstore", strlen(" backingstore")+1);
	int errCode = system(command);
	free(command);

	//close original file and open the dup one in backing store, the dup file is called script_n, n = script number
	fclose(fp);

    char fn[100] = "backingstore/script_";
    char buff[10];
    sprintf(buff, "%d", scriptNum++);
    strcat(fn, buff);

    char* rename = (char*) calloc(1, 200); 
    strcat(rename, "mv ");
    strcat(rename, "backingstore/");
    strcat(rename, filename);
    strcat(rename, " ");
    strcat(rename, fn);

    errCode = system(rename);
    free(rename);

	// char fn[100] = "backingstore/";
	// strcat(fn, newfilename);             //fn is file name
                                                                //Start of code for file manipulation
    FILE *file = fopen(fn, "r");
    if (file == NULL) {
        printf("Error: Unable to open the file.\n");
        exit(1);
    }


    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);


    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL) {
        printf("Error: Unable to allocate memory.\n");
        fclose(file);
        exit(1);
    }

    fread(buffer, 1, fileSize, file);
    buffer[fileSize] = '\0';
    fclose(file);

    replace_semicolons(buffer);

    file = fopen(fn, "w");
    if (file == NULL) {
        printf("Error: Unable to open the file for writing.\n");
        free(buffer);
        exit(1);
    }

    fwrite(buffer, 1, fileSize, file);
    fclose(file);
    free(buffer);
                                                                //End of code here for file manipulation
	//open file from backing store
	FILE* newfile = fopen(fn, "rt");
     
    // load the first 2 pages 
    int totalScriptLines = load_first_2_pages(newfile, fn);
    //printFrameMemory();
    if(error_code != 0){
        fclose(newfile);
        return error_code;
    }
    //first frame = start frame
    *start = framenumbtbl[0];
    //set default value
    
    // for (int x = 0; x < (sizeof(framenumbtbl)/sizeof(framenumbtbl[0])); x++){
    //     // if ptable[x+1] < 0 it means that ptable[x] is the last frame number in page table
    //     // since default values of frame numbers for empty slots is -1
    //     if (framenumbtbl[x+1] < 0) {
    //         *end = framenumbtbl[x];
    //         break;
    //         }
    // }
    
    PCB* newPCB = makePCB(*start,fn);

    //store the total script lines
    newPCB->end = totalScriptLines;
    newPCB->job_length_score = 1+newPCB->end-*start;
    // copy the frame numbers into the PCB
    for (int x = 0; x < (sizeof(framenumbtbl)/sizeof(framenumbtbl[0])); x++){
        if (framenumbtbl[x] < 0) break;
        else{
            newPCB->pagetbl[x]->frameNumber = framenumbtbl[x];
        }
    }

    //change the PC, program counter to have current PAGE to execute, not LINE
    newPCB->PC = newPCB->pagetbl[0]->frameNumber;

    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;
    lock_queue();
    ready_queue_add_to_tail(node);
    unlock_queue();
    fclose(newfile);
    return error_code;
}

//DONT CARE ABOUT SHELL PROCESS FOR A3 CHECK TEST CASES
int shell_process_initialize(){
    //Note that "You can assume that the # option will only be used in batch mode."
    //So we know that the input is a file, we can directly load the file into ram
    int* start = (int*)malloc(sizeof(int));
    int* end = (int*)malloc(sizeof(int));
    int error_code = 0;
    error_code = load_file(stdin, start, end, "_SHELL");
    if(error_code != 0){
        return error_code;
    }
    PCB* newPCB = makePCB(*start,"_SHELL");
    newPCB->priority = true;
    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;
    lock_queue();
    ready_queue_add_to_head(node);
    unlock_queue();
    freopen("/dev/tty", "r", stdin);
    return 0;
}


bool execute_process(QueueNode *node, int quanta){
    char *line = NULL;
    PCB *pcb = node->pcb;
    // printf("script length: %d\n", node->pcb->end);
    // printf("pcb id : %d\n pcb start : %d\n", pcb->pid, pcb->start);
    //this counts the number of lines executed, if it reaches 3, it means that the page is finished
    //change frame if current page is finished
    for(int i=0; i<quanta; i++){            //quanta will be 2
        //current frame and fcounter
        //check if we need to change frame number
        if((pcb->fcounter >= 3)){
            //check if the number of lines executed corresponds to the total # of lines in the script
            if(pcb->lines_executed >= pcb->end){
                //terminate_process(node);
                in_background = false;
                //printf("Ending Process at new iteration with filename: %s \n",pcb->filename);
                return true;
            }
            //otherwise, set PC to be the next page to execute
            else{
                for(int y = 0; y < (sizeof(pcb->pagetbl)/sizeof(pcb->pagetbl[0])); y++) {           //Page table loop check
                    //printf("pcb->pagetbl[%d]->frameNumber: %d\n",y, pcb->pagetbl[y]->frameNumber);
                }
                for(int y = 0; y < (sizeof(pcb->pagetbl)/sizeof(pcb->pagetbl[0])); y++){
                    //printf("Page change of process having filename: %s \n", pcb->filename);
                    if (pcb->pagetbl[y]->frameNumber == pcb->PC){
                        //printf("Seg fault checker1234: \n");
                        //need to check if the next page has been stored
                        //if not stored, need to load more and pop the node from head to tail
                        //printf("Value of frame number as below: %d \n",pcb->pagetbl[y+1]->frameNumber );
                        if(pcb->pagetbl[y+1]->frameNumber < 0){                 //When this is false, break for loop and reset fcounter
                           //NEED TO IMPLEMENT**** 
                            FILE* reOpenFile = fopen(pcb->filename, "rt");
                            //printf("Before Load_one_Page filename: %s \n", pcb->filename);
                            //printf("Before Load_one_Page lines executed: %d \n", pcb->lines_executed);
                            int newframeLoaded = load_one_page((reOpenFile), (pcb->filename), (pcb->lines_executed), pcb);
                            fclose(reOpenFile);
                            //store the new frame number
                            //printf("newFrameLoaded: %d \n", newframeLoaded);
                            //printFrameMemory();
                            pcb->pagetbl[y+1]->frameNumber = newframeLoaded;
                            pcb->PC = pcb->pagetbl[y+1]->frameNumber;
                            pcb->fcounter = 0;                                      //abd added
                            //printf("filename: %s \n", pcb->filename);
                            //printf("PCB->PC: %d \n", pcb->PC);
                            //printf("PCB->fcounter: %d \n", pcb->fcounter);
                            replaceFirst((pcb->PC)*3);
                           
                            //print_LRU();
                            //print_ready_queue();
                           
                            // ready_queue_pop_head();
                            // ready_queue_add_to_tail(node);
                            //printf("loaded more page with frame number: %d!\n", pcb->PC);
                            //stop executing since we moved node to tail, execute next node in queue
                            if (quanta == 2) {return false;}
                            else {break;}
                            
                        }
                        //next page and keep going
                        
                        pcb->PC = pcb->pagetbl[y+1]->frameNumber;
                        break;
                    }
                }
            }
            //reset frame counter
            pcb->fcounter = 0;
        }
        //handle case: code ends in the middle of a frame
        if (strcmp(frame_mem_get_value_at_line((pcb->PC)*3 + pcb->fcounter), "none") == 0){
            /*
            printf("terminate middle page \n");
            printf("filename: %s \n", pcb->filename);
            printf("PCB->PC: %d \n", pcb->PC);
            printf("PCB->fcounter: %d \n", pcb->fcounter);
            printf("Line itself: %s \n", frame_mem_get_value_at_line(1));
            */
            //terminate_process(node);
            //printf("Ending Process with filename: %s \n",pcb->filename);
            in_background = false;
            return true;
        }
        replaceFirst((pcb->PC)*3);
        //print_LRU();                                                         ///Sets the current file as the MRU
        line = frame_mem_get_value_at_line((pcb->PC)*3 + pcb->fcounter);
        in_background = true;
        if(pcb->priority) {
            pcb->priority = false;
        }
        //print_ready_queue();
        parseInput(line);
        in_background = false;
        pcb->fcounter = pcb->fcounter + 1;
        pcb->lines_executed = pcb->lines_executed + 1;
        //if (pcb->fcounter == 3) {printf("pcb->fcounter: %d \n", pcb->fcounter);}
        //printf("pcb->fcounter: %d \n", pcb->fcounter);  
        //printf("pcb->lines_executed: %d \n", pcb->lines_executed);
    }
    return false;
}

void *scheduler_FCFS(){
    QueueNode *cur;
    while(true){
        lock_queue();
        if(is_ready_empty()) {
            unlock_queue();
            if(active) continue;
            else break;   
        }
        cur = ready_queue_pop_head();
        unlock_queue();
        execute_process(cur, MAX_INT);
    }
    if(multi_threading) pthread_exit(NULL);
    return 0;
}

void *scheduler_SJF(){
    QueueNode *cur;
    while(true){
        lock_queue();
        if(is_ready_empty()) {
            unlock_queue();
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        unlock_queue();
        execute_process(cur, MAX_INT);
    }
    if(multi_threading) pthread_exit(NULL);
    return 0;
}

void *scheduler_AGING_alternative(){
    QueueNode *cur;
    while(true){
        lock_queue();
        if(is_ready_empty()) {
            unlock_queue();
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        ready_queue_decrement_job_length_score();
        unlock_queue();
        if(!execute_process(cur, 1)) {
            lock_queue();
            ready_queue_add_to_head(cur);
            unlock_queue();
        }   
    }
    if(multi_threading) pthread_exit(NULL);
    return 0;
}

void *scheduler_AGING(){
    QueueNode *cur;
    int shortest;
    sort_ready_queue();
    while(true){
        lock_queue();
        if(is_ready_empty()) {
            unlock_queue();
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_head();
        shortest = ready_queue_get_shortest_job_score();
        if(shortest < cur->pcb->job_length_score){
            ready_queue_promote(shortest);
            ready_queue_add_to_tail(cur);
            cur = ready_queue_pop_head();
        }
        ready_queue_decrement_job_length_score();
        unlock_queue();
        if(!execute_process(cur, 1)) {
            lock_queue();
            ready_queue_add_to_head(cur);
            unlock_queue();
        }
    }
    if(multi_threading) pthread_exit(NULL);
    return 0;
}

void *scheduler_RR(void *arg){
    //check frame mem contents
    // printFrameMemory();
    int quanta = ((int *) arg)[0];
    QueueNode *cur;
    while(true){
        //print_ready_queue();
        lock_queue();
        if(is_ready_empty()){
            unlock_queue();
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_head();
        unlock_queue();
        if(!execute_process(cur, quanta)) {
            lock_queue();
            ready_queue_add_to_tail(cur);
            unlock_queue();
        }
    }
    if(multi_threading) pthread_exit(NULL);
    return 0;
}

int threads_initialize(char* policy){
    active = true;
    multi_threading = true;
    int arg[1];
    pthread_mutex_init(&queue_lock, NULL);
    if(strcmp("FCFS",policy)==0){
        pthread_create(&worker1, NULL, scheduler_FCFS, NULL);
        pthread_create(&worker2, NULL, scheduler_FCFS, NULL);
    }else if(strcmp("SJF",policy)==0){
        pthread_create(&worker1, NULL, scheduler_SJF, NULL);
        pthread_create(&worker2, NULL, scheduler_SJF, NULL);
    }else if(strcmp("RR",policy)==0){
        arg[0] = 2;
        pthread_create(&worker1, NULL, scheduler_RR, (void *) arg);
        pthread_create(&worker2, NULL, scheduler_RR, (void *) arg);
    }else if(strcmp("AGING",policy)==0){
        pthread_create(&worker1, NULL, scheduler_AGING, (void *) arg);
        pthread_create(&worker2, NULL, scheduler_AGING, (void *) arg);
    }else if(strcmp("RR30", policy)==0){
        arg[0] = 30;
        pthread_create(&worker1, NULL, scheduler_RR, (void *) arg);
        pthread_create(&worker2, NULL, scheduler_RR, (void *) arg);
    }
}

void threads_terminate(){
    if(!active) return;
    bool empty = false;
    while(!empty){
        empty = is_ready_empty();
    }
    active = false;
    pthread_join(worker1, NULL);
    pthread_join(worker2, NULL);
}


int schedule_by_policy(char* policy, bool mt){
    if(strcmp(policy, "FCFS")!=0 && strcmp(policy, "SJF")!=0 && 
        strcmp(policy, "RR")!=0 && strcmp(policy, "AGING")!=0 && strcmp(policy, "RR30")!=0){
            return 15;
    }
    if(active) return 0;
    if(in_background) return 0;
    int arg[1];
    if(mt) return threads_initialize(policy);
    else{
        if(strcmp("FCFS",policy)==0){
            scheduler_FCFS();
        }else if(strcmp("SJF",policy)==0){
            scheduler_SJF();
        }else if(strcmp("RR",policy)==0){
            arg[0] = 2;
            scheduler_RR((void *) arg);
        }else if(strcmp("AGING",policy)==0){
            scheduler_AGING();
        }else if(strcmp("RR30", policy)==0){
            arg[0] = 30;
            scheduler_RR((void *) arg);
        }
        return 0;
    }
}

