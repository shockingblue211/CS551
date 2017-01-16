// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Renjian Jiang

#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define  LARGEST 33554432
//make these variables global so it can be killed by terminating function.
int qid; 
int shmid;        /* segment id of shared memory segment */

//process table unit structure
typedef struct 
{
   pid_t compid;
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


#define KEY (key_t) 40274       
sharedMemory *sharedMem;     /* pointer to shared array, no storage yet*/

// 5. kill compute process, sleep 5 sec, deallocate shared memory
void terminate(int signum){
	printf("Terminating all processes...\n");
	//Kill all the running computes:
	int search;
	for(search=0; search<19;search++){
		if (sharedMem->processTable[search].compid != 0) 
			kill(sharedMem->processTable[search].compid, SIGINT);
	}	
	// sleep 5 seconds
	sleep(5);

	// free shared memory
	if (shmdt((sharedMemory *) sharedMem) == -1) {
		perror("shmdt");
        exit(1);
	}

	if (shmctl(shmid,IPC_RMID,0) == -1){
		perror("shmctl");
        exit(1);
	}
	
	msgctl(qid, IPC_RMID, NULL);
	printf("All processes successfully terminated.\n");
	exit(0);
}


int main(int argc, char const *argv[])
{
	void terminate();
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
	

	// Tasks:
	// 1. generate shared memory;
	if ((shmid = shmget(KEY, sizeof(sharedMemory), IPC_CREAT|0660)) == -1){
		perror("shmget");
        exit(1);
	}

    if ((sharedMem=((sharedMemory *) shmat(shmid,0,0)))== (sharedMemory *) -1) {
            perror("shmat");
            exit(2);
    }

    // clear non-zero bits in the shared memory bitmap. only need to be done once, can't do it more than one time.
    int reset, numPerfect, numProcess;
    for (reset=0; reset< (LARGEST/8); reset++) sharedMem->bitmap[reset] = 0;
    //Set perfect number table and process table to 0, which indicate available space:
    for (reset=0; reset< 20; reset++){
    	sharedMem->perfect[reset] = 0;
    	sharedMem->processTable[reset].compid=0; 
    	sharedMem->processTable[reset].tested=0; 
    	sharedMem->processTable[reset].found=0; 
    	sharedMem->processTable[reset].skipped=0; 
    }

    numPerfect = 0;
    numProcess = 0;
    pid_t managePid = getpid();
    sharedMem->processTable[19].compid = managePid;
    printf("Shared memory allocated and initialized.\n");


    // 2. build a message Q:

    if ((qid=msgget(KEY,IPC_CREAT |0660))== -1) {
        perror("msgget");
        exit(1);
    }

	// 3. assign row for compute
	// 4. update perfect number table
	msgbuf my_msg;
    while(1){
	    if (msgrcv(qid,&my_msg,sizeof(my_msg.mnum),1,0) == -1) {
	    	printf("Error: Failure in receiving message in manage.\n");
	    	exit(1);
	    }


	    if (my_msg.mnum[0] == 0){//find empty row. If all in use, Kill compute sending request.
	    	if (numProcess == 19){
	    		printf("Error: Already 19 processes running. Current compute will be closed.\n");
	    		kill(my_msg.mnum[1], SIGTERM);
	    	}
	    	else{
	    		my_msg.type = my_msg.mnum[1];
		    	long search;
		    	for(search=0; search<19;search++){
		    		if (sharedMem->processTable[search].compid == 0) {
		    			sharedMem->processTable[search].compid = my_msg.mnum[1];
				    	my_msg.mnum[1] = search;//change the mnum to be sent back. mpid should still be the same
			    	    if (msgsnd(qid,&my_msg,sizeof(my_msg.mnum),0) == -1){//send back the empty process table row index
			    	    	printf("Error: Failure in sending the empty row index back to compute\n"); 
			    	    	exit(1);
			    	    }
			    	    numProcess++;
			    	    printf("Now %d process running in total.\n", numProcess);
		    			break;
		    		}
		    	}	
	    	}	    	
	    }
	    else if (my_msg.mnum[0] > 0){//update perfect numbe table
			int k,l,swp, ins;
			ins = (int)my_msg.mnum[1];
			for(k=0;k<numPerfect;k++) if (sharedMem->perfect[k] >= ins) break;//update index
			// if (sharedMem->perfect[k] > ins){//if equal, don't do anything.
				for(l=k;l<numPerfect;l++){
					swp = sharedMem->perfect[l];
					sharedMem->perfect[l] = ins;
					ins = swp;
				}
				sharedMem->perfect[numPerfect++] = ins;	
			// }

			for(l=0;l<numPerfect;l++)
				printf("perfect[%d] = %d.\n", l, sharedMem->perfect[l]);
			if (numPerfect == 20) break;//actually will never happen. Only 5 perfect number within the range.
	    }
	    else if (my_msg.mnum[0] == -1) numProcess--;
    }
}
