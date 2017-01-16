// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Renjian Jiang

#define  LARGEST 33554432
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

// make these variables global so that they can be used in terminate.
int shmid;        /* segment id of shared memory segment */
pid_t pid;
int ptableIndex;
int qid;

typedef struct 
{
   pid_t    compid;
   long tested;
   long found;
   long skipped;	 
} comput_process;

//shared memory structure
typedef struct 
{
   char bitmap [LARGEST/8];
   int perfect [20];
   comput_process processTable [20];	 
} sharedMemory;

typedef struct {
   long type;       
   long mnum[2];    
} msgbuf;

sharedMemory *sharedMem;     /* pointer to shared array, no storage yet*/
msgbuf my_msg;
#define KEY (key_t) 40274     

void setbit(char* bitmapStart, int pointer){
	bitmapStart[pointer/8] |= (1 << (pointer%8));  
}  

int testbit(char* bitmapStart, int pointer){
	int testresult = ((bitmapStart[pointer/8] & (1 << (pointer%8))) != 0);  
	return testresult;
}    

void terminate(int signum){
	// writte to row 20:
	sharedMem->processTable[19].tested = sharedMem->processTable[ptableIndex].tested;
	sharedMem->processTable[19].found = sharedMem->processTable[ptableIndex].found;
	sharedMem->processTable[19].skipped = sharedMem->processTable[ptableIndex].skipped;

	// reset current row:
	sharedMem->processTable[ptableIndex].compid= 0;
	sharedMem->processTable[ptableIndex].tested = 0;
	sharedMem->processTable[ptableIndex].found = 0;
	sharedMem->processTable[ptableIndex].skipped = 0;

	my_msg.type= 1;
	my_msg.mnum[0] = -1;
	if (msgsnd(qid,&my_msg,sizeof(my_msg.mnum),0) == -1) {
				    	printf("Error: send compute terminate failure\n");
				    	exit(1);
	}

	// free shared memory
	if (shmdt((sharedMemory *) sharedMem) == -1) {
		perror("shmdt");
        exit(1);
	}

	printf("Compute process %ld successsfully Terminated.\n", getpid());
	exit(0);
}

//handling terminate signal when all process table are full:
void terminate2(int signum){
	printf("Process table full. Compute process %ld terminated.\n", getpid());
	exit(0);
}

int main (int argc, char const *argv []){
	//void terminate();
	sigset_t mask;
	struct sigaction action;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGHUP);
    action.sa_flags=0;
    action.sa_mask=mask;	

    action.sa_handler=terminate;
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGQUIT, &action, NULL);
	sigaction(SIGHUP, &action, NULL);

    action.sa_handler=terminate2;
	sigaction(SIGTERM, &action, NULL);


	// 1. Chech if arguements are correct.
	if (argc<2){
		printf("Error, Specify the first number to test.\n");
		exit(-1);
	}

	int start;

	if ((start = atoi(argv[1]))== 0){
		printf("Error, wrong specification of first number : %s\n", argv[1]);
		exit(-1);
	}

	if (start > LARGEST){
		printf("Error, starting number larger than bitmap capacity: %s\n", argv[1]);
		exit(-1);
	}

    printf("Start calculating perfect number from: %d\n", start);


	// 2. Attach to shared memory
	if ((shmid = shmget(KEY, sizeof(sharedMemory), IPC_CREAT|0660)) == -1){
		perror("shmget");
        exit(1);
	}

    if ((sharedMem=((sharedMemory *) shmat(shmid,0,0)))== (sharedMemory *) -1) {
            perror("shmat");
            exit(2);
    }

    // 3. build a message Q
    if ((qid=msgget(KEY,IPC_CREAT |0660))== -1) {
        perror("msgget");
        exit(1);
    }

    //send pid and 0 to request the empty row in process table:     
    my_msg.type= 1;
    my_msg.mnum[0]= 0;
    my_msg.mnum[1]= getpid();
    if (msgsnd(qid,&my_msg,sizeof(my_msg.mnum),0) == -1) {
    	printf("Error: process table row request failure\n");
    	exit(1);
    }

    // receive the available row:
    if(msgrcv(qid,&my_msg,sizeof(my_msg.mnum),pid,0) == -1) {
    	printf("Error: process table row receive failure\n");
    	exit(1);
    }
    else printf("Suceed in receiving process table index at row: %ld.\n", my_msg.mnum[1]);
    ptableIndex = (int)my_msg.mnum[1];


    // clear mnum again:
    // my_msg.mnum = 0;
    // 4. calculate perfect numbers
	int current, divsor, sum;
	for (current=start;current<= LARGEST;current++){
		if (testbit(sharedMem->bitmap, current) == 0){//calculate or skip
			if (current == 1) {
				setbit(sharedMem->bitmap, current);//set as tested
				continue; //by definition 1 is not a perfect number
			}
			else{
		        sum=1;
		        for (divsor=2;divsor<current;divsor++)
		                if (!(current%divsor)) sum+=divsor;
		        if (sum==current) {
		        	printf("%d is perfect\n",sum);
		        	my_msg.type= 1;
		        	my_msg.mnum[0]= 1;
		        	my_msg.mnum[1]= (long)sum;
		            if (msgsnd(qid,&my_msg,sizeof(my_msg.mnum),0) == -1) {
				    	printf("Error: send perfect number failure\n");
				    	exit(1);
				    }
				    else printf("Perfect number %ld is successsfully sent to manage.\n", my_msg.mnum[1]);
				    sharedMem->processTable[ptableIndex].found++; 
				}
				setbit(sharedMem->bitmap, current);//set as tested
				sharedMem->processTable[ptableIndex].tested++; 
	    	}
		}
		else sharedMem->processTable[ptableIndex].skipped++; 
	}

	for (current=1;current<start;current++){//wrap around
		if (testbit(sharedMem->bitmap, current) == 0){//calculate or skip
			if (current == 1) {
				setbit(sharedMem->bitmap, current);//set as tested
				continue; //by definition 1 is not a perfect number
			}
			else{
		        sum=1;
		        for (divsor=2;divsor<current;divsor++)
		                if (!(current%divsor)) sum+=divsor;
		        if (sum==current) {
		        	printf("%d is perfect\n",sum);
		        	my_msg.type= 1;
		        	my_msg.mnum[0]= 1;
		        	my_msg.mnum[1]= (long)sum;
		            if (msgsnd(qid,&my_msg,sizeof(my_msg.mnum),0) == -1) {
				    	printf("Error: send perfect number failure\n");
				    	exit(1);
				    }
				    else printf("Perfect number %ld is successsfully sent to manage.\n", my_msg.mnum[1]);
				    sharedMem->processTable[ptableIndex].found++; 
				}
				setbit(sharedMem->bitmap, current);//set as tested
				sharedMem->processTable[ptableIndex].tested++; 
	    	}
		}
		else sharedMem->processTable[ptableIndex].skipped++; 
    } 

    //if wrap around is alos done, send a kill signal to the compute itself to terminate.
   	printf("perfect number from 1 to 2^25 all tested, current process ends");
	kill(getpid(), SIGINT);
}	 
