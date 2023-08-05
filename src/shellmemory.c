#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>
#include "pcb.h"

#define SHELL_MEM_LENGTH 1000
//create a global frame number array so that the function that returns the array doesnt return a pointer
int framenumbtbl[] = {
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
 -1, -1, -1, -1, -1};


char * frame_mem_get_value_at_line(int index);
void frame_mem_free_lines_between(int start, int end);

//Struct for LRU CACHE
struct LRU_LL {
	int framenum;
	struct LRU_LL *next;
};

//Initializing the LRU Cache making the head pointer
struct LRU_LL *first = NULL;

//Function for adding to head
void addFirst(struct LRU_LL *newFrame) {		//Give it a node, and it adds to the head of the LL
	if(!first){
        first = newFrame;
        first->next = NULL;
    }
    else{
        newFrame->next = first;
        first = newFrame;
    }
}
void killLRU() {	//Frees all LRU Memory (done at quit)
	if(!first) return;
    struct LRU_LL *cur = first;
    struct LRU_LL *tmp;
    while(cur->next!=NULL){
        tmp = cur->next;
        free(cur);
        cur = tmp;
    }
    free(cur);
}
int countLRU() {		//Counts the length of the linked list
	int ctr = 0;
	if (!first) return 0;		//Empty LRU

	struct LRU_LL *cur = first;
	while (cur != NULL) {
		ctr++;
		cur= cur->next;
	}
	return ctr;
}
void replaceFirst(int inputFrame) {			//Given a frame number that already exists, it puts the frame as the MRU (Head)
	struct LRU_LL *cur = first;
	struct LRU_LL *tmp1 = NULL;

	if (first->framenum != inputFrame) {	//if it is equal, then its atlready at MRU position, so we dont care
		//Looping through the LL
		while (cur->framenum != inputFrame) {
			tmp1 = cur;	 //Sets tmp pointer to the current
			cur=cur->next; 
		}	//Loop ends when the current has the frame number we need and tmp1 points to the prev node of cur
		//we link tmp1's next to the current's next and add the current to the head
		tmp1->next = cur->next;
		addFirst(cur);		//Now the frame we just used is at the head of the linked list
	}
	return;
}
bool doesExist (int inputFrame) {		//Checks if the input frame already exists in the Linked List
	struct LRU_LL *cur = first;
	while (cur != NULL) {
		if (cur->framenum == inputFrame) {
			return true;
		}
		cur=cur->next;
	}
	return false;
}
int lastNode() {		//Must have atleast 1 node, returns the framenum stored at the tail
	struct LRU_LL *cur = first;
	while (cur->next != NULL) {
		cur=cur->next;
	}
	return cur->framenum;
}
void killLastNode() {
	struct LRU_LL *cur = first;
	struct LRU_LL *tmp1 = NULL;

	if (first->next == NULL) {
		//first is the only node
		first = NULL;
		return;
	}
	while (cur->next != NULL) {
		tmp1 = cur;
		cur=cur->next;
	}
	//Ends when cur->next = null, and tmp1 is the pointer to the prev node
	tmp1->next = NULL;
	return;
}
void print_LRU() {
	if(!first) {
        printf("LRU is empty\n");
        return;
    }
    struct LRU_LL *cur = first;
    printf("LRU: \n");
    while(cur!=NULL){
        printf("\tframeNum: %d\n", cur->framenum);
        cur = cur->next;
    }
}


//for variable store memory
struct memory_struct{
	char *var;
	char *value;
};

//for frame store memory
struct frame_struct{
	int var;
	char *value;
};


//Variable Store memory, this stores all the variables			
struct memory_struct shellmemory[VAR_STORE_SIZE];
//Frame Store memory, this stores all the frame pages
struct frame_struct framememory[FRAME_STORE_SIZE];			

//framememory stores 3 lines of code per slot in the array		//Size will be set in 1.2.2

// Helper functions
int match(char *model, char *var) {
	int i, len=strlen(var), matchCount=0;
	for(i=0;i<len;i++)
		if (*(model+i) == *(var+i)) matchCount++;
	if (matchCount == len)
		return 1;
	else
		return 0;
}

char *extract(char *model) {
	char token='=';    // look for this to find value
	char value[1000];  // stores the extract value
	int i,j, len=strlen(model);
	for(i=0;i<len && *(model+i)!=token;i++); // loop till we get there
	// extract the value
	for(i=i+1,j=0;i<len;i++,j++) value[j]=*(model+i);
	value[j]='\0';
	return strdup(value);
}


// Shell memory functions

void mem_init(){
	int i;
	for (i=0; i<VAR_STORE_SIZE; i++){		
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}
//version for frame mem
//initialize the frame numbers but set values to none
void frame_mem_init(){
	int i;
	for (i=0; i<FRAME_STORE_SIZE; i++){		
		framememory[i].var = i;
		framememory[i].value = "none";
	}
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
	int i;
	for (i=0; i<VAR_STORE_SIZE; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=0; i<VAR_STORE_SIZE; i++){
		if (strcmp(shellmemory[i].var, "none") == 0){
			shellmemory[i].var = strdup(var_in);
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}
	return;
}

//version for frame mem
// Set key value pair
void frame_mem_set_value(int var_in, char *value_in) {
	int i;
	for (i=0; i<FRAME_STORE_SIZE; i++){
		if (framememory[i].var == var_in){
			framememory[i].value = strdup(value_in);
			return;
		} 
	}
	//Value does not exist, need to find a free spot.
	for (i=0; i<FRAME_STORE_SIZE; i++){
		if (strcmp(framememory[i].value, "none") == 0){
			framememory[i].value = strdup(value_in);
			return;
		} 
	}
	return;
}

//get value based on input key
char *mem_get_value(char *var_in) {
	int i;
	for (i=0; i<VAR_STORE_SIZE; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			return strdup(shellmemory[i].value);
		} 
	}
	return NULL;
}

//frame mem version
//get value based on input key
char *frame_mem_get_value(int var_in) {
	int i;
	for (i=0; i<FRAME_STORE_SIZE; i++){
		if (framememory[i].var == var_in){
			return strdup(framememory[i].value);
		} 
	}
	return NULL;
}

void printShellMemory(){				
	int count_empty = 0;
	for (int i = 0; i < VAR_STORE_SIZE; i++){
		if(strcmp(shellmemory[i].var,"none") == 0){
			count_empty++;
		}
		else{
			printf("\nline %d: key: %s\t\tvalue: %s\n", i, shellmemory[i].var, shellmemory[i].value);
		}
    }
	printf("\n\t%d lines in total, %d lines in use, %d lines free\n\n", VAR_STORE_SIZE, VAR_STORE_SIZE-count_empty, count_empty);
}

//frame mem version
void printFrameMemory(){					
	int count_empty = 0;
	for (int i = 0; i < FRAME_STORE_SIZE; i++){
		if(strcmp(framememory[i].value,"none") == 0){
			count_empty++;
		}
		else{
			printf("\nline %d: key: %d\t\tvalue: %s\n", i, framememory[i].var, framememory[i].value);
		}
    }
	printf("\n\t%d lines in total, %d lines in use, %d lines free\n\n", FRAME_STORE_SIZE, FRAME_STORE_SIZE-count_empty, count_empty);
}

/*
 * Function:  addFileToMem 
 * 	Added in A2
 * --------------------
 * Load the source code of the file fp into the shell memory:
 * 		Loading format - var stores fileID, value stores a line
 *		Note that the first 100 lines are for set command, the rests are for run and exec command
 *
 *  pStart: This function will store the first line of the loaded file 
 * 			in shell memory in here
 *	pEnd: This function will store the last line of the loaded file 
 			in shell memory in here
 *  fileID: Input that need to provide when calling the function, 
 			stores the ID of the file
 * 
 * returns: error code, 21: no space left
 */
int get_frame_mem_free_page_OG(){
	int i;
	//variable that tells us if we found empty spot
	bool found = false;
	//find a free page: 3 free spots on frame memory
	for (i=0; i<FRAME_STORE_SIZE-2; i+=3){
		if ((strcmp(framememory[i].value, "none") == 0) &&
		 (strcmp(framememory[i+1].value, "none") == 0) &&
		 (strcmp(framememory[i+2].value, "none") == 0)){
			found = true;
			break;
		} 
	}
	//found = true means we found an empty spot
	if (found) {									///ADd the frame number position to the LRU cache here
		if (doesExist(i)) {
			replaceFirst(i);
			return i;
		}else {
			struct LRU_LL *newFrame = malloc(sizeof(struct LRU_LL));		//Creates a new node in the linked list
			newFrame->framenum = i;
			addFirst(newFrame);			//Adds the new frame to the head of the Linked List
			//printf("Adding %d to the LRU \n", i);
			return i;
		}
		//Whenever found is true, that means that there exists a 3 line wide space in frame memory that is not being used
		//Therefore, we can add it to our linked list which stores the frame indices.
	}											
	//otherwise, need to evict a random page and remove the page table entry in the associated pcb which corresponds to the node we want to
	//evict
	else{
		//get random location, do - 2 because we will evict the random frame numb select and the next 2 as well for a full page
		//int randomEvict = (rand() % (FRAME_STORE_SIZE/3)) + 0;
		// printf("index of eviction : %d \n", randomEvict);
		int randomEvict = lastNode();		//Returns the index of the frame that is at the end
		//killLastNode();
		//give proper print statement for eviction
		printf("Page fault! Victim page contents:\n\n");
		for (int x = randomEvict*3; x<= randomEvict*3 + 2; x++){
			char tmp[100];
			//printf("value of x: %d \n", x);
			strcpy(tmp, frame_mem_get_value_at_line(x));
			//printf("loop1test\n");
			if (strcmp(tmp, "none") != 0){
				printf("%s\n", frame_mem_get_value_at_line(x));
			}
		}
		printf("\nEnd of victim page contents.\n");
		//print_LRU();
		//printFrameMemory();
		//free the page selected
		frame_mem_free_lines_between(randomEvict*3, randomEvict*3 + 2);
		//after evicting, return the random index selected
		//printf("randomEvict: %d \n", randomEvict);
		//replaceFirst(randomEvict);
		//print_LRU();
		//printFrameMemory();
		return randomEvict;
	}
}

//returns start of free frame
int get_frame_mem_free_page(PCB *tbr){
	int i;
	//variable that tells us if we found empty spot
	bool found = false;
	//find a free page: 3 free spots on frame memory
	for (i=0; i<FRAME_STORE_SIZE-2; i+=3){
		if ((strcmp(framememory[i].value, "none") == 0) &&
		 (strcmp(framememory[i+1].value, "none") == 0) &&
		 (strcmp(framememory[i+2].value, "none") == 0)){
			found = true;
			break;
		} 
	}
	//found = true means we found an empty spot
	if (found) {									///ADd the frame number position to the LRU cache here
		if (doesExist(i)) {
			replaceFirst(i);
			return i;
		}else {
			struct LRU_LL *newFrame = malloc(sizeof(struct LRU_LL));		//Creates a new node in the linked list
			newFrame->framenum = i;
			addFirst(newFrame);			//Adds the new frame to the head of the Linked List
			//printf("Adding %d to the LRU \n", i);
			return i;
		}
		//Whenever found is true, that means that there exists a 3 line wide space in frame memory that is not being used
		//Therefore, we can add it to our linked list which stores the frame indices.
	}											
	//otherwise, need to evict a random page and remove the page table entry in the associated pcb which corresponds to the node we want to
	//evict
	else{
		//get random location, do - 2 because we will evict the random frame numb select and the next 2 as well for a full page
		//int randomEvict = (rand() % (FRAME_STORE_SIZE/3)) + 0;
		// printf("index of eviction : %d \n", randomEvict);
		int randomEvict = lastNode();		//Returns the index of the frame that is at the end
		//printf("index of eviction : %d \n", randomEvict);
		//Removes page table entry so no chance of duplicates ever
		for(int y = 0; y < (sizeof(tbr->pagetbl)/sizeof(tbr->pagetbl[0])); y++) {
			if (tbr->pagetbl[y]->frameNumber == randomEvict/3) {
				tbr->pagetbl[y]->frameNumber = -1;
			}
		}
		int len;				//temp var
		//killLastNode();
		//give proper print statement for eviction
		printf("Page fault! Victim page contents:\n\n");
		for (int x = randomEvict; x<= randomEvict + 2; x++){				//Changed from x= randomevict*3 to this
			char tmp[100];
			//printf("value of x: %d \n", x);
			strcpy(tmp, frame_mem_get_value_at_line(x));
			//Fix tmp having newline at the end..........
			len = strlen(tmp);
			if (len > 0 && tmp[len-1] == '\n') {
				tmp[len-1] = '\0';
			}
			//printf("loop1test\n");
			if (strcmp(tmp, "none") != 0){
				printf("%s\n", frame_mem_get_value_at_line(x));
			}
		}
		printf("\nEnd of victim page contents.\n");
		//print_LRU();
		//printFrameMemory();
		//free the page selected
		frame_mem_free_lines_between(randomEvict, randomEvict + 2);		//Here too
		//after evicting, return the random index selected
		//printf("randomEvict: %d \n", randomEvict);
		//replaceFirst(randomEvict);
		//print_LRU();
		//printFrameMemory();
		return randomEvict;
	}
}

//because we are using global variable, need to reset its contents after we passed the frame numbers
void reset_frame_table(){
	for (int x = 0; x < 35; x++){
		framenumbtbl[x] = -1;
	}
	return;
}

int load_file(FILE* fp, int* pStart, int* pEnd, char* filename)
{
	char *line;
    size_t i;
    int error_code = 0;
	bool hasSpaceLeft = false;
	bool flag = true;
	i=101;
	size_t candidate;
	while(flag){
		flag = false;
		for (i; i < FRAME_STORE_SIZE; i++){
			if(strcmp(shellmemory[i].var,"none") == 0){
				*pStart = (int)i;
				hasSpaceLeft = true;
				break;
			}
		}
		candidate = i;
		for(i; i < FRAME_STORE_SIZE; i++){
			if(strcmp(shellmemory[i].var,"none") != 0){
				flag = true;
				break;
			}
		}
	}
	i = candidate;
	//shell memory is full
	if(hasSpaceLeft == 0){
		error_code = 21;
		return error_code;
	}
    for (size_t j = i; j < FRAME_STORE_SIZE; j++){
        if(feof(fp))
        {
            *pEnd = (int)j-1;
            break;
        }else{
			line = calloc(1, SHELL_MEM_LENGTH);
            fgets(line, 999, fp);
			shellmemory[j].var = strdup(filename);
            shellmemory[j].value = strndup(line, strlen(line));
			free(line);
        }
    }
	//no space left to load the entire file into shell memory
	if(!feof(fp)){
		error_code = 21;
		//clean up the file in memory
		for(int j = 1; i <= FRAME_STORE_SIZE; i ++){
			shellmemory[j].var = "none";
			shellmemory[j].value = "none";
    	}
		return error_code;
	}
	//printShellMemory();
    return error_code;
}


//load page version 1 : load the whole file for 1.2.1, instead of loading 1 page at a time
//load all the frame numbers
int load_full_page(FILE* fp, char* filename)
{	
	reset_frame_table();
	int framecount = 0;
	char *line;
    int error_code = 0;
	//frame memory index	
	//int i = get_frame_mem_free_page();			//Fix this if u want to use load_full_page
	int i=0;
	framenumbtbl[framecount] = i / 3;
	// linecount values: 0, 1, 2
	int linecount = 0;
	//total lines keeps track of total number of lines in a script
	int totalLine = 0;
	//no space left to load the entire file into shell memory
	while (!feof(fp)){

		//if the frame of 3 lines is still not filled in frame memory
		if (linecount < 3){
			line = calloc(1, SHELL_MEM_LENGTH);
			fgets(line, 999, fp);
			framememory[i+linecount].value = strndup(line, strlen(line));
			free(line);
			linecount++;
			totalLine++;
		}
		//if all 3 lines of code are filled in the frame, find next free frame
		else{
			//i = get_frame_mem_free_page();			//fix this too
			i=0;
			framecount++;
			framenumbtbl[framecount] = i / 3;
			linecount = 0;
		}
	}
    return error_code;
}


//second version of load to load, this will load a single page FOR 1.2.2
//returns int corresponding to total number of lines stored
int load_first_2_pages(FILE* fp, char* filename)
{																				
	reset_frame_table();
	int framecount = 0;
	char *line;
    int error_code = 0;
	//frame memory index	
	int i = get_frame_mem_free_page_OG();

	//Adding the frame number (i) to the Linked List at the head

	framenumbtbl[framecount] = i / 3;
	// linecount values: 0, 1, 2
	int linecount = 0;
	//total lines keeps track of total number of lines in a script
	int totalLine = 0;
	//no space left to load the entire file into shell memory
	while (!feof(fp)){
		//load while we still havent reached 2 pages
		if (totalLine  < 6){
			//if the frame of 3 lines is still not filled in frame memory
		if (linecount < 3){
			line = calloc(1, SHELL_MEM_LENGTH);
			fgets(line, 999, fp);
			//check for one liners
			char* oneliner[100];
			int a = 0;
			int b;
			char tmp[200];
			while(line[a] != '\n' && line[a] != '\0' && a<1000 && a<strlen(line)) {
				if(line[a] == '\0') break;
				for(b=0; line[a]!=';' && line[a]!='\0' && line[a]!='\n'&& a<strlen(line); a++, b++) tmp[b] = line[a];
				tmp[b] = '\0';
				if(strlen(tmp)==0) continue;
				if(line[a]==';' || line[a] == '\n' || line[a] == '\0'){
					//add the seperate command to frame mem
					if (totalLine < 6){
						if (linecount < 3){
							// printf("oneliner command: %s\n", tmp);
							framememory[i+linecount].value = strndup(tmp, strlen(tmp));
							linecount++;
							totalLine++;
						}
						//if all 3 lines of code are filled in the frame, find next free frame
						else{
							i = get_frame_mem_free_page_OG();
							//Add to LRU

							framecount++;
							framenumbtbl[framecount] = i / 3;
							linecount = 0;
						}
					}
					//if we already stored 2 pages, only continue counting number of lines remaining
					else{
						line = calloc(1, SHELL_MEM_LENGTH);
						fgets(line, 999, fp);
						free(line);
						totalLine++;
					}
					a++;
					for(; line[a]==' ' && a<1000; a++);        // skip white spaces
					continue;
				}
				a++; 
			}
			free(line);
		}
		//if all 3 lines of code are filled in the frame, find next free frame
		else{
			i = get_frame_mem_free_page_OG();
			//Add LRU
			framecount++;
			framenumbtbl[framecount] = i / 3;
			linecount = 0;
		}
		}
		//if we already stored 2 pages, only continue counting number of lines remaining
		else{
			line = calloc(1, SHELL_MEM_LENGTH);
			fgets(line, 999, fp);
			free(line);
			totalLine++;
		}
	}
    return totalLine;
}

//function used to load only a single page, only used when raising page faults
//function returns the frame number value for the new page stored
int load_one_page(FILE* fp, char* filename, int buffer, PCB *tbr)
{	
	int frameValue = -1;
	char *line;
	//frame memory index	
	int i = get_frame_mem_free_page(tbr);
	frameValue = i / 3;
	// linecount values: 0, 1, 2
	int linecount = 0;
	int lineread = 1;
	
	
	//no space left to load the entire file into shell memory
	while (!feof(fp)){
		line = calloc(1, SHELL_MEM_LENGTH);
		fgets(line, 999, fp);
		//load while we still havent reached 2 pages
			//if the frame of 3 lines is still not filled in frame memory
		if (linecount < 3){
			if (lineread > buffer){
				//Abd temp vars
				char *tmp;
				int len;
				//printf("lineread value : %d\n i+linecount value : %d \n", lineread, i+linecount);
				tmp = strndup(line, strlen(line));
				len = strlen(tmp);
				if (len > 0 && tmp[len-1] == '\n') {
					tmp[len-1] = '\0';
				}
				//framememory[i+linecount].value = strndup(line, strlen(line));
				framememory[i+linecount].value = tmp;
				//printf("line itself: %s \n", line);
				free(line);
				linecount++;
			}
			// line = calloc(1, SHELL_MEM_LENGTH);
			// fgets(line, 999, fp);
			// framememory[i+linecount].value = strndup(line, strlen(line));
			// free(line);
			// linecount++;
			lineread++;
		}
		//stop reading of we read a full page
		else break;

	}
	// printFrameMemory();
    return frameValue;
}



char * mem_get_value_at_line(int index){
	if(index<0 || index > VAR_STORE_SIZE) return NULL; 
	return shellmemory[index].value;
}

//frame store version
char * frame_mem_get_value_at_line(int index){
	if(index<0 || index > FRAME_STORE_SIZE) return NULL; 
	return framememory[index].value;
}


void mem_free_lines_between(int start, int end){
	for (int i=start; i<=end && i<VAR_STORE_SIZE; i++){
		if(shellmemory[i].var != NULL){
			free(shellmemory[i].var);
		}	
		if(shellmemory[i].value != NULL){
			free(shellmemory[i].value);
		}	
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

void frame_mem_free_lines_between(int start, int end){				///Freeing the strdup memory here to kill leaks
	for (int i=start; i<=end && i<FRAME_STORE_SIZE; i++){

		if(strcmp(framememory[i].value, "none") != 0){
			free(framememory[i].value);								//ABD Added this
			framememory[i].value = "none";
		}	
	}
}
