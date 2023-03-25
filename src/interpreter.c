#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "shellmemory.h"
#include "shell.h"

int MAX_ARGS_SIZE = 7;

struct dirent *dp;

//PCB STRUCT
//process id 4 = run
//process id 1, 2, 3 = exec
struct pcb{
	int pid;
	//starting key of script
	char head[4];
	int length;
	int scorelength;
	char instruct[100];
};

struct qNode{		//As a linked list
	int isFin;		//if this is 1, then the process is completed
	struct pcb *process;		//The process associated with the current block
	struct qNode *next;		//Goes to the next process
};



int badcommand(){
	printf("%s\n", "Unknown Command");
	return 1;
}

// For run command only
int badcommandFileDoesNotExist(){
	printf("%s\n", "Bad command: File not found");
	return 3;
}

//For too many tokens
int badcommandToken(){
	printf("%s \n", "Bad command: Too many tokens");
	return 4;
}

//For my_mkdir only
int badcommandmkdir(){
	printf("%s \n", "Bad command: my_mkdir");
	return 2;
}

//For my_cd only
int badcommandcd(){
	printf("%s \n", "Bad command: my_cd");
	return 5;
}

//for dup exec programs
int badExec(){
	printf("%s \n", "Bad command: same file name");
	return 1;
}

//for bad policy
int badPolicy(){
	printf("%s \n", "Bad command exec: Invalid Policy");
}

int help();
int quit();
int set(char* var, char* value);
int print(char* var);
int run(char* script);
int badcommandFileDoesNotExist();
int echo(char* var);
char *getcwd(char *buf, size_t size);
int chdir(const char* var);
int my_ls();
int my_mkdir(char* var);
int my_touch(char* var);
int my_cd(char* var);
int my_exec(char* progs[], char* policy, int background);



//compare function for sorting the files in my_ls, not the right one
int my_strcmp( const void* a, const void* b )
{
  return strcmp( *(const char**)a, *(const char**)b );
}

// Interpret commands and their arguments
int interpreter(char* command_args[], int args_size){
	int i;
	// printf("INFINITE LOOP! \n");
	if ( args_size < 1) return badcommand();
	if (args_size > MAX_ARGS_SIZE) return badcommandToken();
	for ( i=0; i<args_size; i++){ //strip spaces new line etc
		command_args[i][strcspn(command_args[i], "\r\n")] = 0;
	}

	if (strcmp(command_args[0], "help")==0){
	    //help
	    if (args_size != 1) return badcommand();
	    return help();
	
	} else if (strcmp(command_args[0], "quit")==0) {
		//quit
		if (args_size != 1) return badcommand();
		return quit();

	} else if (strcmp(command_args[0], "set")==0) {
		//set
		//MAX_ARGS_SIZE handles the upper bound case where we exceed 5 tokens for 1 variable
		//case if there isnt enough tokens
		if ((args_size < 3)) return badcommand();

		char result[505];
		strcpy(result, command_args[2]);
		
		for (i = 3; i < args_size; i ++){
			//restore the spaces between tokens
			strcat(result, " ");
			strcat(result, command_args[i]);
		}	
		return set(command_args[1], result);
		
	
	} else if (strcmp(command_args[0], "print")==0) {
		if (args_size != 2) return badcommand();
		return print(command_args[1]);
	
	} 
	//CHANGE FOR 1.2.1
	else if (strcmp(command_args[0], "run")==0) {
		if (args_size != 2) return badcommand();
		
		return run(command_args[1]);
	
	}
	
	 else if (strcmp(command_args[0], "echo")==0){
		if (args_size != 2) return badcommand();
		return echo(command_args[1]);	
	} 
	//my_ls
	else if (strcmp(command_args[0], "my_ls") == 0){
		//my_ls is the only token needed
		if (args_size != 1) return badcommand();
		return my_ls();
	}
	//my_mkdir
	else if (strcmp(command_args[0], "my_mkdir") == 0){
		if (args_size != 2) return badcommand();
		return my_mkdir(command_args[1]);
	}
	//my_touch
	else if (strcmp(command_args[0], "my_touch") == 0){
		//only allowing 2 arguments: my_touch and the file name
		if (args_size != 2) return badcommand();
		return my_touch(command_args[1]);
	}
	//my_cd
	else if (strcmp(command_args[0], "my_cd") == 0){
		//only allowing 2 arguments: my_cd and the directory name
		if (args_size != 2) return badcommand();
		return my_cd(command_args[1]);
	}

	//exec
	else if (strcmp(command_args[0], "exec") == 0){
		// printf("Here before call to exec\n");
		//exec can take between 1 and 3 programs + argument for policy and possible # tag
		char policy[6];
		if (args_size < 3 || args_size > 6) return badcommand();

		//if == 0, then no #, if == 1 then there is #
		int isBackGround = 0;
		char* progs[] = {"none", "none", "none"};

		strcpy(policy, command_args[args_size-1]);			//Change when doing MT

		//check for #
		if (strcmp(policy, "#") == 0) {
			// if there is, set isBackGround to 1 and set correct value for policy
			isBackGround = 1;
			strcpy(policy, command_args[args_size-2]);
			}

			//check if works
			// printf("policy with a # : %s", policy);
			//Mmake sure that command_args[i] contains a filename and not a policy
		if (args_size >= 3 && (strcmp(command_args[1], "FCFS") != 0) 
		&& (strcmp(command_args[1], "SJF") != 0) 
		&& (strcmp(command_args[1], "RR") != 0)
		&& (strcmp(command_args[1], "AGING") != 0)) 
		{	
			// printf("command_args[1] is : %s \n", command_args[1]);
			progs[0] = strdup(command_args[1]);}
		if (args_size >= 4 && (strcmp(command_args[2], "FCFS") != 0) 
		&& (strcmp(command_args[2], "SJF") != 0) 
		&& (strcmp(command_args[2], "RR") != 0)
		&& (strcmp(command_args[2], "AGING") != 0)) 
		{progs[1] = strdup(command_args[2]);}
		if (args_size >= 5 && (strcmp(command_args[3], "FCFS") != 0) 
		&& (strcmp(command_args[3], "SJF") != 0) 
		&& (strcmp(command_args[3], "RR") != 0)
		&& (strcmp(command_args[3], "AGING") != 0)) 
		{progs[2] = strdup(command_args[3]);}
		// printf("Here before call to exec\n");


		// if (args_size == 6){}
		return my_exec(progs, policy, isBackGround);

	}

	 else return badcommand();
}

int help(){

	char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
	printf("%s\n", help_string);
	return 0;
}

int quit(){
	printf("%s\n", "Bye!");
	exit(0);
}

int set(char* var, char* value){
	char *link = "=";
	char buffer[1000];
	strcpy(buffer, var);
	strcat(buffer, link);
	strcat(buffer, value);

	mem_set_value(var, value);

	return 0;

}

int print(char* var){
	printf("%s\n", mem_get_value(var)); 
	return 0;
}
//MODIFIED
int run(char* script){
	int errCode = 0;
	char line[1000];
	FILE *p = fopen(script,"rt");  // the program is in a file

	if(p == NULL){
		return badcommandFileDoesNotExist();
	}

	fgets(line,999,p);
	//load the script into shell memory
	//"800"-"829" are the contiguous names for each script line
	int contName = 800;
	char buffer[10];

	//code loading
	while(1){
		//convert int to string
		snprintf (buffer, sizeof(buffer), "%d",contName);
		// printf(" contname: %s \n", buffer);
		mem_set_value(buffer,line);
		memset(line, 0, sizeof(line));
		contName++;
		if(feof(p)){
			break;
		}
		fgets(line,999,p);
	}

	int scriptlength = contName - 800;
	//create instance of PCB
	struct pcb* runPCB = (struct pcb*) malloc(sizeof(struct pcb));
	runPCB->pid = 8;
	strcpy(runPCB->head, "800");
	//put into head of queue
	runPCB->length = scriptlength;
	runPCB->scorelength = scriptlength;
	strcpy(runPCB->instruct, mem_get_value("800"));


	//Now run the process
	
	for (int i = (int) strtol(runPCB->head, (char **)NULL, 10); i < 800+runPCB->length;i++){
		snprintf (buffer, sizeof(buffer), "%d",i);
		strcpy(runPCB->instruct, mem_get_value(buffer));

		//FOR INITIAL TESTING if we encounter quit, we dont want to quit the shell, just want to quit process
		// if (strcmp(runPCB.instruct, "quit") == 0) printf("%s\n", "Bye!");
		errCode = parseInput(runPCB->instruct);
	}
    fclose(p);

	//clear shell memory, removes the lines of code in the shell
	for (int i = (int) strtol(runPCB->head, (char **)NULL, 10); i < 800+runPCB->length;i++){
		snprintf (buffer, sizeof(buffer), "%d",i);
		mem_remove(buffer);
	}

	//Need to free the runPCB as well
	free(runPCB);
	return errCode;
}



//assumption tokens have <100 char length
int echo(char* var){
	char result[100];
	strcpy(result, var);
	if (result[0] == '$'){
		char *varname = malloc(99); 
		strncpy(varname, result+1, 99);
		printf("%s \n", mem_get_value(varname));
	}
	else printf("%s \n", var);
		return 0;
}

//my_ls implementation
//the following stack-overflow post was used as reference to implement my_ls : https://stackoverflow.com/questions/28862372/unix-command-ls-in-c
int my_ls(){
	char cwd[1000];
		//getcwd(currentdir, sizeof(currentdir));
		if (getcwd(cwd, sizeof(cwd)) != NULL){
			DIR *dir = opendir(cwd);
			char* filelist[100];
			int index = 0;
			//get all file names into filelist
			while((dp = readdir(dir)) != NULL){
				if ((strcmp(dp->d_name, ".") == 0) || (strcmp(dp->d_name, "..") == 0)) continue;
					filelist[index] = (dp->d_name);
				
				index++;
			}
			//sort filelist
			// size_t size = sizeof(filelist) / sizeof(filelist[0]);
			qsort(filelist, index, sizeof(filelist[0]), my_strcmp);
			
			for (int j = 0; j < index; j++){
				printf("%s \n", filelist[j]);
			}
			return 0;
		}
		else return badcommand();
}

//my_mkdir implementation
//used this site as reference to implement the creation of the directory:
//https://www.geeksforgeeks.org/create-directoryfolder-cc-program/
int my_mkdir(char* var){
	char vardup[100];
	strcpy(vardup, var);
	char dirname[100];
	char temp[100];
	char *token[100]; //used in the case that we have a $ to retrieve the token
	//check if there is a $
	if (vardup[0] == '$'){
		char *varname = malloc(99); //99 since we assumed var length is at most 100 char and minus the $ char we get 99
		strncpy(varname, vardup+1, 99);
		strcpy(dirname, mem_get_value(varname));
		//need to check if the string in memory is only 1 token
		//use the same code as parseInput to turn the token(s) into the same format as command_args in interpreter
		int a = 0;
		int b;
		int w = 0;
		for(a=0; dirname[a]==' ' && a<1000; a++);        // skip white spaces
    	while(dirname[a] != '\n' && dirname[a] != '\0' && a<1000) {
        for(b=0; dirname[a]!=';' && dirname[a]!='\0' && dirname[a]!='\n' && dirname[a]!=' ' && a<1000; a++, b++){
            temp[b] = dirname[a];                        
            // extract a word
        }
		temp[b] = '\0';
        token[w] = strdup(temp);
        w++;
        if(dirname[a] == '\0') break;
        a++;
		}
		//check 2 cases: no token or too many tokens
		if (w > 1 || (strcmp(token[0], "Variable does not exist") == 0)) return badcommandmkdir();
		strcpy(dirname, token[0]) ;
		
	} else{
		strcpy(dirname, vardup);
	}
	int fail = mkdir(dirname, 0777);
	//if mkdir returns non zero value, error
	if (fail){ 
		return badcommandmkdir();
	}
	return 0;
}

//my_touch implementation
int my_touch(char* var){
	FILE *fp;
	
	fp = fopen(var, "w");
	fclose(fp);
	return 0;
}

//my_cd implementation
int my_cd(char* var){
	// char cwd[100];
	// getcwd(cwd, sizeof(cwd));
	int fail;
	fail = chdir( (const char*) var);
	//check if the return value of chdir is non zero
	if (fail) return badcommandcd();
	return 0;
}

//my_exec
int my_exec(char* progs[], char* policy, int background){
	// printf("progs[0] : %s \n", progs[0]);
	int proc_counter = 0; //Will tell us how many programs are loaded
	//check for duplicate programs
    if ((strcmp(progs[0], progs[1]) == 0) || (strcmp(progs[0], progs[2]) == 0) || (strcmp(progs[1], progs[2]) == 0 && strcmp(progs[1], "none") != 0)) {
		return badExec();
	}
	
	//code loading
	int size = 3;						//Need to fix this hardcoding for 1.2.5
	int proglength[size];
	int errcode = 0;

	//Starting from -1								
	for (int i = -1; i < size-1; i++){
		proc_counter++;
		//printf("Here before fgets \n");
		if (strcmp(progs[i+1], "none") == 0) break;

		char line[1000];
		// printf("file name : %s \n", progs[i+1]);
		FILE *p = fopen(progs[i+1],"rt");  // the program is in a file


		if(p == NULL){
			return badcommandFileDoesNotExist();
		}
		
		fgets(line,999,p);
		//load the script into shell memory
		//"000"-"129" for prog1, "200"-"329" for prog2, "400"-"529" for prog3
		int contName = (i+1)*200;  //should this be 200?
		char buffer[10];
		//code loading
		while(1){

			//convert int to string
			//Buffer is key, line is value
			snprintf (buffer, sizeof(buffer), "%d",contName);
			errcode = mem_set_value(buffer,line);
			// If mem_set_value does not return properly then we must stop exec

			memset(line, 0, sizeof(line));
			contName++;
			if(feof(p)){
				break;
			}
			fgets(line,999,p);
		}
		//load program length into proglength
		proglength[i+1] = contName - ((i+1)*200);  //should this be 200?
		//close file	
		fclose(p);
		
	}

	
	
	//create the PCB's
	//create the first one seperatly too keep track of the head
	//ABD EDITS
	
	

	//The following code only accomodates FCFS Policy
	//So Far we have made 3 structs which we have to free later, after each process is completed!
	//We dont make current now, we make it later
	


	//create head qnode
	struct qNode *headPtr = (struct qNode*) malloc(sizeof(struct qNode));
	struct pcb *headPCB = (struct pcb*) malloc(sizeof(struct pcb));

	//Initialises the first PCB according to the memory locations
	headPCB->pid = 1;

	headPCB->length = proglength[0];
	headPCB->scorelength = proglength[0];
	strcpy(headPCB->head, "0");
	strcpy(headPCB->instruct, mem_get_value(headPCB->head)); //At init, current instruction will be the instruction where the head pointer is pointing too.

	// printf("headPCB instruct : %s \n", headPCB->instruct);
	//****check if we need to add shell as process****
	if (background){
		//if yes, add shell in shell mem and add it as another PCB in head

		//load shell into shell mem

		//call the prompt simulate terminal or read file and store all inputs into userInput
		char prompt = '$';
		int MAX_USER_INPUT = 1000;
		char userInput[MAX_USER_INPUT];
		strncpy(userInput, "", sizeof(userInput)); 
		int index = 0;
		char* shellInput[1000];

		while(1) {						
        if (isatty(fileno(stdin))) printf("%c ",prompt);
		fgets(userInput, MAX_USER_INPUT-1, stdin);
		if (strstr(userInput, "exec") != NULL) {
			break;}
		shellInput[index] = strdup(userInput);
		index++;
		if (feof(stdin)){
			break;
		}	
		// errcode = parseInput(userInput);
		// if (errcode == -1) exit(99);	// ignore all other errors
		memset(userInput, 0, sizeof(userInput));
		// printf("userInput after memset : %s \n", userInput);
		}

		//load last exec into shell mem
		mem_set_value("999", userInput);

		// check shellInput, Works
		// for (int i = 0; i<index;i++){
		// 	printf("shell : %s \n", shellInput[i]);
		// }

		// load shellInput into shell memory
		int shellMemVal = 900;
		char buffer[10];
		for (int i = 0; i< index; i++){
			snprintf (buffer, sizeof(buffer), "%d",shellMemVal);
			mem_set_value(buffer, shellInput[i]);
			shellMemVal++;
		}
		// printf("shell mem : %s \n", mem_get_value("902"));
		//create shell PCB
		struct pcb *shellPCB = (struct pcb*) malloc(sizeof(struct pcb));
		shellPCB->pid = 0; 
		//*****************************NEED TO CHANGE THIS LENGTH VALUE OF WE MODIFY SHELL.C ************************************
		shellPCB->length = index;
		shellPCB->scorelength = 0; //since shell is always priority, set priority to lowest value, zero
		strcpy(shellPCB->head, "900");
		strcpy(shellPCB->instruct, mem_get_value(shellPCB->head));


		//INCREMENT PROC_COUNTER
		proc_counter++;


		headPtr->isFin = 0;
		headPtr->process = shellPCB;

	
		
		
		//create the corresponding QNode for headPCB as another one
		struct qNode *firstQNode = (struct qNode*) malloc(sizeof(struct qNode));
		firstQNode->isFin = 0;
		firstQNode->process = headPCB;
		firstQNode->next=NULL;

		//set 1st prog after shell
		headPtr->next=firstQNode;

		//create tail PCB and Ptr
		struct pcb *tailPCB = (struct pcb*) malloc(sizeof(struct pcb));
		struct qNode *tailQNode = (struct qNode*) malloc(sizeof(struct qNode));
		strcpy(tailPCB->head, "999"); //just 1 line
		strcpy(tailPCB->instruct, mem_get_value(tailPCB->head));
		tailPCB->pid = 5;
		tailPCB->length = 1;
		tailPCB->scorelength = 1000; //make it go last
		tailQNode->isFin=0;
		tailQNode->process=tailPCB;
		tailQNode->next = NULL;
		


		//check if there is a second prog
	if (strcmp(progs[1], "none") != 0){
		//create 2nd qnode
		struct qNode *ptr1 = (struct qNode*) malloc(sizeof(struct qNode));

		struct pcb *p2PCB = (struct pcb*) malloc(sizeof(struct pcb));
		p2PCB->pid = 2;
		p2PCB->length = proglength[1];
		p2PCB->scorelength = proglength[1];
		strcpy(p2PCB->head, "200");
		strcpy(p2PCB->instruct,mem_get_value(p2PCB->head));

		headPtr->next->next = ptr1;
		ptr1->isFin = 0;
		ptr1->process = p2PCB;

		//check if there is third prog
		if (strcmp(progs[2], "none") != 0){
			//create 3rd qnode
			struct qNode *ptr2 = (struct qNode*) malloc(sizeof(struct qNode));

			struct pcb *p3PCB = (struct pcb*) malloc(sizeof(struct pcb));
			p3PCB->pid = 3;
			p3PCB->length = proglength[2];
			p3PCB->scorelength = proglength[2];
			strcpy(p3PCB->head, "400");
			strcpy(p3PCB->instruct ,mem_get_value(p3PCB->head));

			ptr1->next = ptr2;
			ptr2->isFin = 0;
			ptr2->process = p3PCB;
			ptr2->next = NULL;
	}else {ptr1->next = tailQNode;}
	  } else headPtr->next->next = tailQNode;
	}
	else{
	
		headPtr->isFin = 0;
		headPtr->process = headPCB;
		headPtr->next=NULL;

		//check if there is a second prog
	if (strcmp(progs[1], "none") != 0){
		// printf("here! \n");
		//create 2nd qnode
		struct qNode *ptr1 = (struct qNode*) malloc(sizeof(struct qNode));

		struct pcb *p2PCB = (struct pcb*) malloc(sizeof(struct pcb));
		p2PCB->pid = 2;
		p2PCB->length = proglength[1];
		p2PCB->scorelength = proglength[1];
		strcpy(p2PCB->head, "200");
		strcpy(p2PCB->instruct,mem_get_value(p2PCB->head));

		headPtr->next = ptr1;
		ptr1->isFin = 0;
		ptr1->process = p2PCB;

		//check if there is third prog
		if (strcmp(progs[2], "none") != 0){
			// printf("here2! \n");
			//create 3rd qnode
			struct qNode *ptr2 = (struct qNode*) malloc(sizeof(struct qNode));

			struct pcb *p3PCB = (struct pcb*) malloc(sizeof(struct pcb));
			p3PCB->pid = 3;
			p3PCB->length = proglength[2];
			p3PCB->scorelength = proglength[2];
			strcpy(p3PCB->head, "400");
			strcpy(p3PCB->instruct ,mem_get_value(p3PCB->head));

			ptr1->next = ptr2;
			ptr2->isFin = 0;
			ptr2->process = p3PCB;
			ptr2->next = NULL;
	}else ptr1->next = NULL;
	 } //else headPtr->next = NULL;
	}	
	// printf("headPtr->next id: %d \n", headPtr->next->process->pid);
	// printf("progs[1] : %s \n", progs[1]);
	// printf("proc_counter : %d \n", proc_counter);





	// //check if there is a second prog
	// if (strcmp(progs[1], "none") != 0){
	// 	//create 2nd qnode
	// 	struct qNode *ptr1 = (struct qNode*) malloc(sizeof(struct qNode));

	// 	struct pcb *p2PCB = (struct pcb*) malloc(sizeof(struct pcb));
	// 	p2PCB->pid = 2;
	// 	p2PCB->length = proglength[1];
	// 	p2PCB->scorelength = proglength[1];
	// 	strcpy(p2PCB->head, "200");
	// 	strcpy(p2PCB->instruct,mem_get_value(p2PCB->head));

	// 	headPtr->next = ptr1;
	// 	ptr1->isFin = 0;
	// 	ptr1->process = p2PCB;

	// 	//check if there is third prog
	// 	if (strcmp(progs[2], "none") != 0){
	// 		//create 3rd qnode
	// 		struct qNode *ptr2 = (struct qNode*) malloc(sizeof(struct qNode));

	// 		struct pcb *p3PCB = (struct pcb*) malloc(sizeof(struct pcb));
	// 		p3PCB->pid = 3;
	// 		p3PCB->length = proglength[2];
	// 		p3PCB->scorelength = proglength[2];
	// 		strcpy(p3PCB->head, "400");
	// 		strcpy(p3PCB->instruct ,mem_get_value(p3PCB->head));

	// 		ptr1->next = ptr2;
	// 		ptr2->isFin = 0;
	// 		ptr2->process = p3PCB;
	// 		ptr2->next = NULL;
	// }else ptr1->next = NULL;
	//  } //else headPtr->next = NULL;

	// printf("headPtr->next id: %d \n", headPtr->next->process->pid);
	// printf("head process : %s \n", headPtr->process->instruct);
	// printf("next process : %s\n ", headPtr->next->process->instruct);
	

	//check policy
	if (strcmp(policy, "FCFS") == 0){
	
		//Now we need to traverse the ready queue, using a temporary pcb pointer & temp qnode pointer			TRAVERSAL CODE ################
		struct pcb *tmpPCB = (struct pcb*) malloc(sizeof(struct pcb));
		struct qNode *tmpQNode = (struct qNode*) malloc(sizeof(struct qNode));
		int sm_counter = 0; //Shell memory counter, starts from 0, at each iteration jumps by 200
		char tmpBuffer[10];
		tmpQNode = headPtr;


		// while(tmpQNode->next != NULL){
		while (tmpQNode!= NULL) {
			
			tmpPCB = tmpQNode->process;				//Goes directly to the process at each iteration of the tmpQNode
			sm_counter = strtol(tmpPCB->head, (char **)NULL, 10); //Returns the starting position on the shell memory

			for (int i = (int) strtol(tmpPCB->head, (char **)NULL, 10); i < sm_counter+tmpPCB->length; i++) {
				snprintf(tmpBuffer, sizeof(tmpBuffer), "%d", i);            //Check the thing stored in tmp buffer
				//printf("\n The thing stored in tmp Buffer is: %s \n and the value",tmpBuffer);    // Its stored as 0,1,2 etc instead of 000,001 etc.
				strcpy(tmpPCB->instruct,mem_get_value(tmpBuffer));
				errcode = parseInput(tmpPCB->instruct);
			}
			tmpQNode = tmpQNode->next;				//Goes to the next node of the ready queue
		}

		//clear shell memory, removes the lines of code in the shell
		tmpQNode = headPtr;
		tmpPCB = tmpQNode->process;
		char buffer[10];
		//loop to go through each process
		for (int i = 0; i < proc_counter; i++){
			//loop to go through each lines in a process
			for (int j = (int) strtol(tmpPCB->head, (char **)NULL, 10); j < 800+tmpPCB->length;j++){
			snprintf (buffer, sizeof(buffer), "%d",j);
			mem_remove(buffer);
			}
		}
		//Freeing the ready queue stuff, first the innder processes
		struct qNode *garbage = headPtr;
		while (garbage != NULL){
			free(garbage->process);
			garbage = garbage->next;
		} //Now the main Ready Queue
		while (headPtr != NULL){
			garbage = headPtr;
			headPtr=headPtr->next;
			free(garbage);
		}
		return errcode;

		}
	else if (strcmp(policy, "SJF") == 0){
		struct qNode *tPtr = (struct qNode*) malloc(sizeof(struct qNode));
		struct qNode *tempQPtr;
		struct qNode *newHead ;

		for (int i=0; i<proc_counter; i++) {
			/////////////////////////////////////////////////////HERE, WILL DO SJF And update Head Accordingly/////////////////////////
			tPtr = headPtr;
			int maxScore = -1;         //For comparing the score
			while (tPtr != NULL) {
				if ((tPtr->process->scorelength >= maxScore) && (tPtr->isFin == 0)) {			//Makes sure to only take unfinished processes
					//Noting pid and scorelength and isFin
					maxScore = tPtr->process->scorelength;
					newHead = tPtr;					// Stores the Node's pointer in it temporarily
					
				}

				tPtr = tPtr->next;		//going to the next node
			}
			// printf("reached! \n");
			tPtr = headPtr;
			while (tPtr !=NULL) {
				if (tPtr->next == newHead) {
					tPtr->next = tPtr->next->next;		//Skips over the node we want to make the head
				}
				tPtr=tPtr->next;
			}
			// printf("reached! \n");
			//Now we have a linked list, with newHead isolated and its previous node skipping over it to go to its child
			if (newHead != headPtr) {			//To stop it pointing to itself.
				newHead->next = headPtr;
			}
			newHead->isFin = 1;		///REmove this line in aging
			headPtr = newHead;
			/////////////////////////////////////////////////////HERE, WILL DO SJF And update Head Accordingly////////////////////////
		}
		///Execution Loop starts here, Loop fully sorted, just copy implementation of FCFS here now as things are properly allocated in LL
		int sm_counter=0;
		char tmpBuffer[4];
		tPtr = headPtr;
		struct pcb *tmpPCB;
		//seg fault issue?
		while (tPtr != NULL) {

			tmpPCB = tPtr->process;
			sm_counter = (int) strtol(tmpPCB->head, (char **)NULL, 10);

			for (int i = (int) strtol(tmpPCB->head, (char **)NULL, 10); i < sm_counter+tmpPCB->length; i++) {
				snprintf(tmpBuffer, sizeof(tmpBuffer), "%d", i);
				strcpy(tmpPCB->instruct,mem_get_value(tmpBuffer));
				// printf("tPCB instruct : %s \n",tmpPCB->instruct);
				errcode = parseInput(tmpPCB->instruct);
				// printf("executed! \n");
			}

			tPtr = tPtr->next;
		}

		//Clear Shell Memory
		tPtr = headPtr;
		tmpPCB = tPtr->process;
		char buffer[10];
		for (int i = 0; i < proc_counter; i++){
			//loop to go through each lines in a process
			for (int j = (int) strtol(tmpPCB->head, (char **)NULL, 10); j < 800+tmpPCB->length;j++){
			snprintf (buffer, sizeof(buffer), "%d",j);
			mem_remove(buffer);
			}
		}

		//Freeing the ready queue stuff, first the innder processes
		struct qNode *garbage = headPtr;
		while (garbage != NULL){
			free(garbage->process);
			garbage = garbage->next;
		} //Now the main Ready Queue

		while (headPtr != NULL){
			garbage = headPtr;
			headPtr=headPtr->next;
			free(garbage);
		}
	
		return errcode;

	}
	else if (strcmp(policy, "RR") == 0){

		//Need to switch process after 2 instructions, assume we start from the first process loaded, then switch to the next one until 
		//we reach the null, then we go back to head
		struct pcb *tmpPCB = (struct pcb*) malloc(sizeof(struct pcb));
		struct qNode *tmpQNode = (struct qNode*) malloc(sizeof(struct qNode));					//Need to free these 2 freeing the temp mallocs???
		//headPtr is the head of the Ready Queue
		int sm_counter = 0; //Shell memory counter, starts from 0, at each iteration jumps by 200
		char tmpBuffer[10]; 
		int isOver = 0;			//If this turns 1, then all processes are finished
		
		tmpQNode = headPtr;
		
		while (1) { 
			int finisherCounter = 0;
			while (tmpQNode->isFin == 1) {
				finisherCounter++;
				if (tmpQNode->next == NULL) {tmpQNode = headPtr;}
				else {tmpQNode = tmpQNode->next;}

				if (finisherCounter == proc_counter) {
					isOver=1;
					break;
					}		//If the finisher counter, which counts the number of finished processes is the same as the processor count, then we break all loops
			}
			if (isOver == 1) {break;}	//Ends the while loop if everything is finished

			tmpPCB = tmpQNode->process;				//Goes directly to the process at each iteration of the tmpQNode
			int lengthOfCurrent = tmpPCB->length;
			sm_counter = (int) strtol(tmpPCB->head, (char **)NULL, 10); //Returns the starting position on the shell memory
			//Running 2 instructions:
			for (int i=0; i<2;i++) {
				snprintf(tmpBuffer, sizeof(tmpBuffer), "%d", sm_counter);		//Stores the value of the sm_counter into tmpBuffer as a string
				strcpy(tmpPCB->instruct,mem_get_value(tmpBuffer));				//Evaluates the instruction associated with the tmpBuffer
				errcode = parseInput(tmpPCB->instruct);
				sm_counter++;		//Increments sm_counter by 1 each time the loop is done

				//Need to check if we have not exceeded the length and Finish Condition of each process
				if (sm_counter - ((tmpPCB->pid - 1)*200 ) == tmpPCB->length) {
					//Comes here, when the current instruction to be executed - head number is equal to the length, 3 instructions at 200-202
					tmpQNode->isFin = 1; //Sets finish bit of the current process, and exits the for loop
					break;

				} //Got the (modified) head, stored stuff in the buffer, executed the instruction, got back the errcode, increased sm, finish check
					//Finish checks for each process
			}
			//End of for loop, need to store the sm_counter as the new head of the pcb
			//Now we modify the head back to the new instruction
			if (tmpQNode->isFin == 1) {
				//Finished, reset the head to the OG Instruction
				sprintf(tmpPCB->head, "%03d", ((tmpPCB->pid - 1)*200));
			} else {
				sprintf(tmpPCB->head, "%03d", sm_counter);			//Makes head the new sm_counter
			}
			//Now one process has finished executing for 2 turns, now we switch to the next process
			
			if (tmpQNode->next == NULL) {
				tmpQNode = headPtr;
			}else {
				tmpQNode = tmpQNode->next;
			}
		} //End of While
		
		//Clearing Shell Memory
		tmpQNode = headPtr;
		tmpPCB = tmpQNode->process;
		char buffer[10];
		//loop to go through each process
		for (int i = 0; i < proc_counter; i++){
			//loop to go through each lines in a process
			for (int j = (int) strtol(tmpPCB->head, (char **)NULL, 10); j < 800+tmpPCB->length;j++){
				snprintf (buffer, sizeof(buffer), "%d",j);
				mem_remove(buffer);
			}
		}
		
		//Freeing the ready queue stuff, first the innder processes
		struct qNode *garbage = headPtr;
		while (garbage != NULL){
			free(garbage->process);
			garbage = garbage->next;
		} //Now the main Ready Queue
		while (headPtr != NULL){
			garbage = headPtr;
			headPtr=headPtr->next;
			free(garbage);
		}
		return errcode;

	}
	
	else if (strcmp(policy, "AGING") == 0){
		struct qNode *tPtr = (struct qNode*) malloc(sizeof(struct qNode));
		struct qNode *tempHPtr;
		struct qNode *newHead;

		///Sorting LL Initially:
		for (int i=0; i<proc_counter; i++) {
			tPtr=headPtr;
			tempHPtr=headPtr;

			//Skipping first i nodes
			for (int j=0; j<i;j++) {
				tempHPtr=tPtr;
				tPtr=tPtr->next;
			}
			int minSS = 150;
			while (tPtr != NULL) {
				if ((tPtr->process->scorelength < minSS) && (tPtr->isFin == 0)) {			//Makes sure to only take unfinished processes
					minSS = tPtr->process->scorelength;
					newHead = tPtr;					// Stores the Node's pointer in it temporarily
				}
				tPtr=tPtr->next;		//going to the next node
			}
			//newHead is the pointer to the value of the next smallest scorelength value
			//Need to attach newHead after i nodes now
			tPtr=headPtr;
			while (tPtr !=NULL) {								////First, we make sure that the LL is good without the node in question
				if (tPtr->next == newHead) {
					tPtr->next = tPtr->next->next;		//Skips over the node we want to make the head
				}
				tPtr=tPtr->next;
			}
			//tempHPtr will be the head of the LL/ or the node behind the current tPtr
			if (newHead == headPtr) {
				headPtr=newHead;
				continue;
			} else if ((newHead != headPtr) && (i < 1)){		//First iteration
				newHead->next = headPtr;
				headPtr=newHead;
			} else if ((newHead != headPtr) && (i >= 1)) {		//All iterations after the first
				newHead->next = tempHPtr->next;
				tempHPtr->next = newHead;
			}											///Look at Diagram on ABD ipad
		}

			//Now the LL is sorted
		int isOver = 0;			//Breaks the while loop

		while (isOver == 0) {
			/////////////////////////////////////////////////////HERE, WILL DO SJF And update Head Accordingly/////////////////////////
			tPtr = headPtr;
			int minScore = 150;         //For comparing the score
			while (tPtr != NULL) {
				if ((tPtr->process->scorelength < minScore) && (tPtr->isFin == 0)) {			//Makes sure to only take unfinished processes
					//Noting pid and scorelength and isFin
					minScore = tPtr->process->scorelength;
					newHead = tPtr;					// Stores the Node's pointer in it temporarily
				}
				tPtr=tPtr->next;		//going to the next node
			}
			//current process to execute
			tPtr = headPtr;
			while (tPtr !=NULL) {
				if (tPtr->next == newHead) {
					tPtr->next = tPtr->next->next;		//Skips over the node we want to make the head
				}
				tPtr=tPtr->next;
			}
			//Now we have a linked list, with newHead isolated and its previous node skipping over it to go to its child
			if (newHead != headPtr) {			//If newHead is not the head already, then we take the head and push it down to the tail
				//Storing the OG headPtr in tempHPtr
				tempHPtr = headPtr;
				newHead->next = tempHPtr->next;	// Sets the order of nodes to the next for newHead to maintain the linked list
				//Traversing the LL to put headPtr at the tail, which will the tPtr with the next value null
				tPtr = headPtr;
				while (tPtr != NULL) {
					if (tPtr->next == NULL) {										
						tPtr->next = headPtr;
						headPtr->next = NULL;		
					}
					tPtr = tPtr->next;
				}
				// printf("PID of newHead is: %d\n", newHead->process->pid);
				// printf("Score Length of newHead is %d \n \n", newHead->process->scorelength);
				// printf("PID of next of newHead is: %d\n", newHead->process->pid);
				// printf("Score Length of next of newHead is %d \n \n", newHead->process->scorelength);
				// printf("PID of next of next of newHead is: %d\n", newHead->process->pid);
				// printf("Score Length of next of next of newHead is %d \n \n", newHead->process->scorelength);
			}
			headPtr = newHead;
			/////////////////////////////////////////////////////HERE, WILL DO SJF And update Head Accordingly/////////////////////////

			//Once the LL has been set, we decrement all the processes job strenght till 0 one by one except the head
			tPtr = headPtr->next;

			while (tPtr != NULL) {
				if (tPtr->process->scorelength > 0) {
					tPtr->process->scorelength = tPtr->process->scorelength - 1;
				}
				tPtr = tPtr->next;
			}

			//Counting how many processes have finished
			int finProcs = 0;
			tPtr = headPtr;
			while (tPtr != NULL) {
				if (tPtr->isFin == 1) {
					finProcs++;
				}
				tPtr = tPtr->next;
			}


			//Now we Process the head just once: headPtr
			int sm_counter=0;
			char tmpBuffer[4];
			tPtr = headPtr;
			struct pcb *tmpPCB;

			tmpPCB = headPtr->process;
			int lengthOfCurrent = tmpPCB->length;
			sm_counter = (int) strtol(tmpPCB->head, (char **)NULL, 10);

			snprintf(tmpBuffer, sizeof(tmpBuffer), "%d", sm_counter);		
			strcpy(tmpPCB->instruct,mem_get_value(tmpBuffer));				//Processing the head Node once
			errcode = parseInput(tmpPCB->instruct);
			sm_counter++;

			if (sm_counter - ((tmpPCB->pid - 1)*200 ) == tmpPCB->length) {
				headPtr->isFin = 1;									//Checking to see if the program finishes
				finProcs++;
				if (finProcs == proc_counter) {
					isOver = 1;										//Sets Terminating Condition
				}
			}

			//Rewrites the sm_counter into the head of the PCB & cleanup
			if (headPtr->isFin == 1) {
				//Finished, reset the head to the OG Instruction
				sprintf(tmpPCB->head, "%03d", ((tmpPCB->pid - 1)*200));
			} else {
				sprintf(tmpPCB->head, "%03d", sm_counter);			//Makes head the new sm_counter
			}


		}	//While Loop Ends here


		//Cleanup Stuff
		//Clear Shell Memory
		struct pcb *tmpPCB;
		tPtr = headPtr;
		tmpPCB = tPtr->process;
		char buffer[10];
		for (int i = 0; i < proc_counter; i++){
			//loop to go through each lines in a process
			for (int j = (int) strtol(tmpPCB->head, (char **)NULL, 10); j < 800+tmpPCB->length;j++){
			snprintf (buffer, sizeof(buffer), "%d",j);
			mem_remove(buffer);
			}
		}

		//Freeing the ready queue stuff, first the innder processes
		struct qNode *garbage = headPtr;
		while (garbage != NULL){
			free(garbage->process);
			garbage = garbage->next;
		} //Now the main Ready Queue

		while (headPtr != NULL){
			garbage = headPtr;
			headPtr=headPtr->next;
			free(garbage);
		}

		return errcode;

	}
	else if (strcmp(policy, "RR30") == 0) {
		//Copy RR2, with the for loop changed to 30 and this will work	

	}
	else return badPolicy();
	
}