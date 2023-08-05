#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H
#include <stdbool.h>
#include "pcb.h"
void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int load_file(FILE* fp, int* pStart, int* pEnd, char* fileID);
char * mem_get_value_at_line(int index);
void mem_free_lines_between(int start, int end);
void printShellMemory();
//functions for frame store
void frame_mem_init();
void frame_mem_set_value(char *var_in, char *value_in);
char *frame_mem_get_value(char *var_in);
void printFrameMemory();
char * frame_mem_get_value_at_line(int index);
int *load_full_page(FILE* fp, char* filename);
int get_frame_mem_free_page(PCB *tbr);
int get_frame_mem_free_page_OG();
void frame_mem_free_lines_between(int start, int end);
int load_first_2_pages(FILE* fp, char* filename);
int load_one_page(FILE* fp, char* filename, int buffer, PCB *tbr);
//Functions for LRU Cache
//void addFirst(struct LRU_LL *newFrame);
void killLRU();
int countLRU();
void replaceFirst(int inputFrame);
bool doesExist (int inputFrame);
void killLastNode();
void print_LRU();

//global variable used to pass array of frame numbers, useful for 1.2.1 , 1.2.2
extern int framenumbtbl[35];
#endif
