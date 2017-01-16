// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - RENJIAN JIANG
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>
#include <time.h>


int fastread(int readfd, int* netNum){
    int hostint;
    read(readfd, netNum, sizeof(int));
    hostint = ntohl(*netNum);
    return hostint;
}

void fastreadvoid(int readfd, int* netNum){
    read(readfd, netNum, sizeof(int));
    *netNum = ntohl(*netNum);
}

void fastwrite(int writefd, int* LocNum){
    int netNum = htonl(*LocNum);
    // printf("Number to fastwrite is %d\n", netNum );
    write(writefd,&netNum, sizeof(int));
}

void errmsg (char *em){
	printf("Error: %s\n", em);
	exit (-1);
}

typedef struct{
	int finished;
	int start;
	int range;
	int sec;
	int current;
	int PerfNum;
	int curThreadNum; 
	int termination;
} WorkMem;

WorkMem *WorkMemArry; //global struct array
int sfd; //make socket fd global
int threadNum;
int threadTermed=0; //the whole process knows how many working threads there are.
int type; //for MComm to catch
int wrBuf;
int action[2];
int todo;
sigset_t set;
int  sig;

pthread_t SigID, SendID, RecvID;
pthread_t *threadID;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


void recvNewRange(void){
	int index;
	fastreadvoid(sfd,&index);	
	fastreadvoid(sfd,&(WorkMemArry[index].start));
	fastreadvoid(sfd,&(WorkMemArry[index].range));
	printf("Received new range with start %d and range %d for thread %d.\n",WorkMemArry[index].start,WorkMemArry[index].range,index);
	WorkMemArry[index].finished = 0;	
}

void TerminateFunc(void){
	int t;
	for(t= 0;t<threadNum;t++){
		WorkMemArry[t].termination = 1;
	}
}

void mutxwithSig (int actiontype, int index){

		while(todo){
			pthread_mutex_unlock(&mtx);
			pthread_mutex_lock(&mtx);
		}
		action[0] = actiontype;//new range
		action[1] = index;
		todo = 1;//fill in todo
		pthread_cond_signal(&cond);
}

void mutxwithSig2 (int actiontype, int index){
	pthread_mutex_lock(&mtx);//initiate the lock
		while(todo){
			pthread_mutex_unlock(&mtx);
			pthread_mutex_lock(&mtx);
		}
		action[0] = actiontype;//new range
		action[1] = index;
		todo = 1;//fill in todo
		pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mtx);                   
}

void * Receive(){
	int byte; 
	while(1){
		byte = read(sfd, &wrBuf, sizeof(int));
		pthread_mutex_lock(&mtx);//initiate the lock
			type = ntohl(wrBuf);
			// printf("Received type %d\n", type);
	        if (byte!= sizeof(int)){
			    printf("WARNING: Lost connection.\n");
			    // Colse the connection in this case?
			    close(sfd);
			    exit(-1);
	        }
	       	// sent from manage:
			//2. new range to work on 7.request of worker's currents 
			//9. termination request 11. linklist remove confirmation 

	        else if(type == 2){//new range
	        	printf("Received a new range.\n");
	        	recvNewRange();
	        	pthread_cond_signal(&cond);
	        }	
	        else if(type == 7){//request of worker's currents
	        	printf("Received a request of worker's currents.\n");
	        	mutxwithSig(5,0);//action type 5, index doesn't matter
	        }
	        else if(type == 9){//termination request
	        	printf("Received a termination request.\n");
	        	kill(getpid(),SIGINT);
	        	pthread_cond_signal(&cond);
	        }  
	        else if(type == 11){//linklist remove confirmation
	        	printf("Received a linklist remove confirmation.\n");
				threadTermed++;//acumulate the number of thread ready to terminate
				pthread_cond_signal(&cond);
	        }
	        else {
	        	printf("Error: type = %d.\n", type);  
	        	pthread_cond_signal(&cond);
	        }
    	pthread_mutex_unlock(&mtx);                   
	}
}

void * Sig(){
	int s,k;


	if ((s = sigwait(&set, &sig)) != 0) errmsg("sigwait");
	printf("Signal handling thread got signal %d\n", sig);
	
	TerminateFunc();

	while(1){
		if(threadTermed == threadNum){
			printf("All terminated range removed from manage, terminating work thread...\n");
			for(k = 0; k < threadNum; k++){
//TODO: could cause problem here				
				pthread_join(((pthread_t)threadID[k]),NULL);
			}
			printf("All %d work thread terminated.\n", threadNum);

//TODO: Why don't join thread here?			
			// printf("Terminating send and Receive thread...\n");
			// pthread_join(SendID,NULL);
		 //    pthread_join(RecvID,NULL);
			// printf("Send and Receive thread terminated.\n");
			printf("Exit.\n");

		    exit(0);
		}
	}
}

//original type protocol:
//1.range request 2. new range to work on 3. range finished 
//4.worker's currents 5. perfect numbers 6. range terminated
//7.request of worker's currents 8.report's request 
//9. termination request 10. -k 11. linklist remove confirmation 

//sent from compute:
//1.range request 3. range finished 
//4.worker's currents 5. perfect numbers 6. range terminated

// sent from manage:
//2. new range to work on 7.request of worker's currents 
//9. termination request 11. linklist remove confirmation 

//sent from report(the same type never reaches compute)
//8.report's request (changed by manage to 7 to compute)
//10. -k (changed by manage to 9 to compute)

void * Send(){

    pthread_mutex_lock(&mtx);//initiate the lock
      while (1) {
	    while (todo == 0) {//check wait condition, if met, release lock and wait
	            pthread_cond_wait(&cond,&mtx);//wait until the signal, go back to see if while loop condition met again
	            }
	    //type of actions: 1. new range 2. range finished 3. terminating 4. perfect number 5. send workers current
   		if(action[0] == 1){//new range

			type = 1;
			//write type to manage
			printf("Type to send is: %d\n", type);
			fastwrite(sfd,&type);
			fastwrite(sfd,&action[1]); //thread index 
			printf("Sent a new range request for newly initiated thread %d\n", action[1]);
   			todo=0;//reset todo
   		}   
   		else if (action[0] == 2){//range finished

			type = 3;
			//write type to manage
			fastwrite(sfd,&type);
			fastwrite(sfd,&action[1]); //thread index 
			fastwrite(sfd,&(WorkMemArry[action[1]].start));//send finished start
			fastwrite(sfd,&(WorkMemArry[action[1]].sec));
   			printf("Sent a range request for thread %d with finished start %d and range %d and type %d.\n", action[1],WorkMemArry[action[1]].start,WorkMemArry[action[1]].range,type );
   			todo=0;
   		}   
   		else if (action[0] == 3){//terminating

   			type = 6;
			//write type to manage
			fastwrite(sfd,&type);
			fastwrite(sfd,&action[1]); //thread index 
			fastwrite(sfd,&(WorkMemArry[action[1]].start));//send terminating start

   			todo=0;
   		}      
   		else if (action[0] == 4){//perfect number

   			type = 5;
			//write type to manage
			fastwrite(sfd,&type);
			fastwrite(sfd,&action[1]);
			printf("Perfect Number %d is sent to manage.\n",action[1]);
   			todo=0;
   		}  
   		else if (action[0] == 5){//send workers current

   			type = 4;
			//write type to manage
			fastwrite(sfd,&type);
			fastwrite(sfd,&threadNum);
			int i,tested,currentStart,end;
			for(i=0;i<threadNum;i++){
				end = WorkMemArry[i].start + WorkMemArry[i].range;
				currentStart = WorkMemArry[i].current;
				tested  = currentStart - WorkMemArry[i].start;

				fastwrite(sfd,&tested);//send terminating start
				fastwrite(sfd,&currentStart);//send terminating start
				fastwrite(sfd,&end);//send terminating start

			}
			
   			todo=0;
   		}  
      }
    pthread_mutex_unlock(&mtx);

}

// Mutex and conditional variable to synchronize work threads startup.
pthread_cond_t  t_start_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t t_start_m  = PTHREAD_MUTEX_INITIALIZER;

void * Work (void *arg){
	int end, sum, divsor, msgsnt;
	time_t Tstart, Tend;
	msgsnt = 0;//
	// Synchronizing startup with the main thread.
    pthread_mutex_lock(&t_start_m);	
	int indx = *(int*)arg;
	// printf("Entered thread %d.\n", indx);
    pthread_cond_signal(&t_start_cv);
    pthread_mutex_unlock(&t_start_m);	

	// printf("finished = %d, start = %d, range = %d, sec = %d, \n", WorkMemArry[indx].finished,WorkMemArry[indx].start,WorkMemArry[indx].range,WorkMemArry[indx].sec );
	// printf("current = %d, perfect = %d, curThreadNum = %d, termination = %d.\n",WorkMemArry[indx].current,WorkMemArry[indx].PerfNum,WorkMemArry[indx].curThreadNum,WorkMemArry[indx].termination );
	while(1){//inifitely ask for new range to calculate
    //type of actions: 1. new range 2. range finished 3. terminating 4. perfect number

		 if(WorkMemArry[indx].finished == -1 && msgsnt == 0){//new work thread start
		 	printf("Initiated thread, use mutex to ask for range.\n");
			mutxwithSig2(1,WorkMemArry[indx].curThreadNum);//action type 1. new range, with thread index
			msgsnt = 1;
		}
		else if (WorkMemArry[indx].finished == 1 && msgsnt == 0){//current range finished calculating
			printf("Finished range, use mutex to ask for range.\n");
			mutxwithSig2(2,WorkMemArry[indx].curThreadNum);//action type 2. range finished, with thread index
			msgsnt = 1;
		}
		else if (WorkMemArry[indx].finished == 0) {//if new range assigned, finished will be changed back to 0 by PComm thread.
			printf("Range updated, start finding perfect number.\n");
			time(&Tstart);
			end = WorkMemArry[indx].start + WorkMemArry[indx].range;
			for (WorkMemArry[indx].current = WorkMemArry[indx].start; WorkMemArry[indx].current <= end; WorkMemArry[indx].current++){
				if (WorkMemArry[indx].termination == 1){
					printf("Received termination message within calculation loop.\n");
					mutxwithSig2(3,WorkMemArry[indx].curThreadNum);//action type 3. terminating, with thread index
					msgsnt = 1;
					break;
				}

		        sum=1;
		        for (divsor=2;divsor<WorkMemArry[indx].current;divsor++)
		                if (!(WorkMemArry[indx].current%divsor)) sum+=divsor;
		        if (sum==WorkMemArry[indx].current) {
		        	printf("%d is perfect\n",sum);
		        	WorkMemArry[indx].PerfNum = sum;//use mutex here?
		        	mutxwithSig2(4,WorkMemArry[indx].PerfNum);
				}
			}

			//prevent small chance event: termination turn to 1 after the range just finished:
			if(WorkMemArry[indx].termination == 1 && msgsnt == 0){
				//send again before sending range finished action.
				printf("Received termination message after calculation loop.\n");
				mutxwithSig2(3,WorkMemArry[indx].curThreadNum);//action type 3. terminating, with thread index
				break;//prevent the while(1) to ask for anything
			}
			else if (WorkMemArry[indx].termination == 1 && msgsnt == 1){
				printf("Termination message sent, break the while(1) loop.\n");
				break;//msg already sent, can just break to avoid any other msg
			} 

	    	//change finished to 1, let Send know this thread needs new range in the next while loop condition 2
		    WorkMemArry[indx].finished = 1;
		    msgsnt = 0;
			printf("Current range done for thread: %d, ask for another range\n", WorkMemArry[indx].curThreadNum);
			time(&Tend);
			WorkMemArry[indx].sec = (int)Tend - Tstart;
		} 
	}
}


int main(int argc, char const *argv[]){

	if (argc != 4) errmsg("Wrong numbe of arguements");
	if ((threadNum = atoi(argv[3])) < 1) errmsg("# of thread < 1");
	printf("Client asks %d thread(s)\n", threadNum);


	WorkMemArry = (WorkMem *)calloc(threadNum, sizeof(WorkMem));
	threadID = (pthread_t*)calloc(threadNum,sizeof(pthread_t));
	int init;
	for(init=0;init<threadNum;init++){
		WorkMemArry[init].finished = -1;
		WorkMemArry[init].start = 0;
		WorkMemArry[init].range = 0;
		WorkMemArry[init].sec = 0;
		WorkMemArry[init].current = 0;
		WorkMemArry[init].PerfNum = 0;
		WorkMemArry[init].curThreadNum = 0;
		WorkMemArry[init].termination = 0;
	}

	// for(init=0;init<threadNum;init++){
	// printf("Global WorkMem %d:\n", init);
	// printf("finished = %d, start = %d, range = %d, sec = %d, \n", WorkMemArry[init].finished,WorkMemArry[init].start,WorkMemArry[init].range,WorkMemArry[init].sec );
	// printf("current = %d, perfect = %d, curThreadNum = %d, termination = %d.\n",WorkMemArry[init].current,WorkMemArry[init].PerfNum,WorkMemArry[init].curThreadNum,WorkMemArry[init].termination );
	// }

	//sigwait, handle termination.

    int s;
    sigemptyset(&set);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGHUP);
    if((s = pthread_sigmask(SIG_BLOCK, &set, NULL)) != 0) errmsg("pthread_sigmask");
	pthread_create(&SigID, NULL, Sig, (void *) &set);
	printf("Signal thread created.\n");

//2. build up connection 
    struct sockaddr_in sin; /* socket address for destination */
    long address;
    int i,j;

    address = *(long *) gethostbyname(argv[1])->h_addr;
    sin.sin_addr.s_addr= address;
    sin.sin_family= AF_INET;
    sin.sin_port = htons(atoi(argv[2]));
    printf("Hostname: %s and port number: %d has been added to address adder\n", argv[1], atoi(argv[2]));

	/*loop waiting for manage if Necessary */
    while(1) {
                    /* create the socket */
    if ((sfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
            perror("Socket");
            exit(1);
            }

            /* try to connect to Mary */
    if (connect (sfd, (struct sockaddr *) &sin, sizeof (sin)) <0) {
            printf("Where is that Manage!\n");
            close(sfd);
            sleep(10);
            continue;
            }
    break; /* connection successful */
    }
    printf("Connection build with manage.\n");
//start Receive thread
    pthread_create(&RecvID, NULL, Receive, NULL);
    printf("Receive thread created.\n");
//start Send thread
    pthread_create(&SendID, NULL, Send, NULL);
    printf("Send thread created.\n");

//start work threads
    int k;
//    int err = pthread_cond_init(&t_start_cv, NULL);
//    if (err) printf("Failed to init conditional var: %d", err);
    pthread_mutex_lock(&t_start_m);
	for(k = 0; k < threadNum; k++){
		// printf("Entered the loop for the %d th time \n", k );
		WorkMemArry[k].finished = -1;//new Work
		WorkMemArry[k].curThreadNum = k;
		// printf("finished and curThreadNum assigned, finished = %d, curThreadNum = %d\n", WorkMemArry[k].finished, WorkMemArry[k].curThreadNum);
    	pthread_create(&threadID[k], NULL, Work,  (void *) &k);
    	// Waiting the just started thread to finish its startup to increment the k 
    	//variable which is the thread's argument (its index).
    	pthread_cond_wait(&t_start_cv, &t_start_m);
    	printf("Work thread %d created.\n", k);
    }
    pthread_mutex_unlock(&t_start_m);
    
    pause();//main thread pause.

}
