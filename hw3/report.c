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
#define KEY (key_t) 40274   

int shmid; 

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
sharedMemory *sharedMem; 

int main(int argc, char const *argv[])
{
	// 1. Attach to shared memory
	if ((shmid = shmget(KEY, sizeof(sharedMemory), IPC_CREAT|0660)) == -1){
		perror("shmget");
        exit(1);
	}

    if ((sharedMem=((sharedMemory *) shmat(shmid,0,0)))== (sharedMemory *) -1) {
            perror("shmat");
            exit(2);
    }

    // 2. Report perfect numbers:
    printf("Perfect numbers found: ");
    int perf = 0;
    while(1){
    	if(sharedMem->perfect[perf] == 0) break;

    	printf("%d ", sharedMem->perfect[perf]);
    	perf++;
    }
    printf("\n");



    // 3. calculate total numbers:
    long testedTotal=0, skippedTotal=0, foundTotal=0;//in case the number summed up are huge, use long
    	int search;
	for(search=0; search<19;search++){
		if (sharedMem->processTable[search].compid != 0) {
			if (argc ==1){//no need to print if is going to kill.
				printf("Row %d is active.", search);
				printf("PID %ld: ", sharedMem->processTable[search].compid);
				printf("tested: %ld, ", sharedMem->processTable[search].tested);
				printf("found: %ld, ", sharedMem->processTable[search].found);
				printf("skipped: %ld.\n", sharedMem->processTable[search].skipped);					
			}
			testedTotal += sharedMem->processTable[search].tested;
			foundTotal += sharedMem->processTable[search].found;
			skippedTotal += sharedMem->processTable[search].skipped;
		}
	}	
	testedTotal += sharedMem->processTable[19].tested;
	foundTotal += sharedMem->processTable[19].found;
	skippedTotal += sharedMem->processTable[19].skipped;
	printf("Total tested: %ld, ", testedTotal);
	printf("total found: %ld, ", foundTotal);
	printf("total skipped: %ld.\n", skippedTotal);	

	//4. kill all process if specified:
	if (argc >1 && strcmp("-k", argv[1])  == 0){
		printf("Killing manage with pid: %ld.\n", sharedMem->processTable[19].compid);
		kill(sharedMem->processTable[19].compid, SIGINT);
	}

}
