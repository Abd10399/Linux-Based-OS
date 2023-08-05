#ifndef PAGING_H
#define PAGING_H
#include <stdbool.h>

typedef struct{
    int frameNumber;
    int pageID;
    // char *pageValue[3];
    bool initialized;
}PAGE;

int generatePageID();
PAGE * makePAGE();
#endif