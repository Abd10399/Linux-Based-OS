#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "paging.h"

int pageID_counter = 1;

int generatePageID(){
    return pageID_counter++;
}

PAGE* makePAGE(){
    PAGE * newPAGE = malloc(sizeof(PAGE));
    newPAGE->pageID = generatePageID();
    //default frame number is -1, means that it is not stored in frame memory yet
    newPAGE->frameNumber = -1;

    //false means the page instance is a placeholder and is not a page we use
    newPAGE->initialized = false;
    //initialize the lines of code in the page as none
    // for (int x = 0; x < 3; x++){
    //     newPAGE->pageValue[x] = "none";
    // }

    return newPAGE;
}