// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - RENJIAN JIANG
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
//#include <stropts.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>




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
    write(writefd,&netNum, sizeof(int));
}

struct pollfd pollarray[20];     /* up to 1+19 simultaneous connections*/                                             /* sum [1..5] for the 5 johns */
int termination = 0;
int type;
int reportfd, reportfdindx;
// 5. kill compute process, sleep 5 sec, deallocate shared memory
void terminate(int signum){
	printf("Terminating all processes...\n");
	//Kill all the running computes:
	termination = 1;
	int j;
    int openedfd = 0;
    for (j=1;j<=19;j++){//all the connections >0 and <i should be computes 
        if(pollarray[j].fd!=-1) {
            openedfd++;   
            // printf("Found a connected compute host at index %d, %d connected compute host found so far.\n",j,openedfd);
        } 
        // else printf("No connection on index %d.\n", j);     
    }
    if (openedfd == 0){
        printf("No compute host is connected, just terminate manage itself.\n");
        exit(0);
    } 
	for (j=1;j<=19;j++){//all the connections >0 and <i should be computes 
		if(pollarray[j].fd!=-1){
    		type = 9;//request for current;
			fastwrite(pollarray[j].fd, &type);//use MComm and mutex to receive this signal?
			printf("Termination messge sent to pollarray index %d\n",j );
		}
	}
	//leave type == 6 in the poll loop to handle the rest of the termination.
}



int main (argc,argv)
        int argc;
        char *argv[];
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

        struct sockaddr_in sin; /* structure for socket address */
        int s;
        int fd;
        int len, i,count, wrBuf,lastStart;
        lastStart = 2;//avoid 1 to start with

        char computehost[20][160];   /*1-19: names of host of computes*/
        
		typedef struct{
			int PerfNum;
			char PerfHost[160];
		} PerfRecord;

		PerfRecord PerfRecords [20];//at most 20 number to be stored (can't find that much)
		int TotalPerfNum= 0; //total number of perfect numbers found

        struct hostent *hostentp;
        int compNum, compSent;

		typedef struct _node{
			int start;
			int range;
			char Host[160];
			struct _node* next;
		} node;//linklist node

		node *head,*pt;//linklist head, linklist pointer

		head= NULL;

                /* set up IP addr and port number for bind */
        sin.sin_addr.s_addr= INADDR_ANY;//what's the difference between binding to local hosting and binding to any interface?
        sin.sin_port = htons(atoi(argv[1]));//integer to short?

                /* Get an internet socket for stream connections */
        if ((s = socket(AF_INET,SOCK_STREAM,0)) < 0) {
                perror("Socket");
                exit(1);
                }

                /* Do the actual bind */
        if (bind (s, (struct sockaddr *) &sin, sizeof (sin)) <0) {
                perror("bind");
                exit(2);
                }

                /* Allow a connection queue for up to 5 JOHNS */
        listen(s,20);
        pollarray[0].fd=s;     /* Accept Socket*/
        pollarray[0].events=POLLIN;
                                                  /* 5 possible john's */
        for (i=1;i<=19;i++) {pollarray[i].fd=-1;pollarray[i].events=POLLIN;}

        while(1) {
                poll(pollarray,20,-1);   /* no timeout, blocks until some activity*/

                                /* Check first for new connection */
                if (pollarray[0].revents & POLLIN) {//1 & 1?

                        len=sizeof(sin);
                        if ((fd= accept (s, (struct sockaddr *) &sin, &len)) <0) {
                                perror ("accept");
                                        exit(3);
                                }
                                        /* Find first available slot for new john */
                        for (i=1;i<=19;i++) if (pollarray[i].fd == -1) break;
                                pollarray[i].fd=fd;
                                hostentp=gethostbyaddr((char *)&sin.sin_addr.s_addr,
                                        sizeof(sin.sin_addr.s_addr),AF_INET);
                                strcpy((char *)&computehost[i][0], hostentp->h_name);
                                printf("Built an connection on array index %d, with host %s.\n",i, computehost[i] );
                        }

                                /* If there are no new connections, process waiting computes */
                else for (i=1;i<=19;i++) {
                        // if((pollarray[i].fd !=-1) && (pollarray[i].revents & POLLHUP)){
                        // 	printf("Connection on poll index %d has close. Remove fd\n", i);
                        // 	close(pollarray[i].fd);
                        //     pollarray[i].fd = -1;
                        //     memset(computehost[i], 0, sizeof(computehost[i]));
                        // }
                        // else 
                        if ((pollarray[i].fd !=-1) && (pollarray[i].revents)) {//check which fd received a message
                        		//The first interger (4 bytes) read in always indicates type.
                        		// printf("Before read, type = %d\n", type );
                                count=read(pollarray[i].fd,&wrBuf,sizeof(int));//need to check EOF here, don't use fast read
                                type = ntohl(wrBuf);
                                // printf("Type = %d\n", type );
                                //If didn't read in an integer, report error and exist. 
                                //Does the scenario of reading in an EOF exist? (see note)
                                if (count!= sizeof(int)){
	                                printf("WARNING: The connection on pollarray index %d could have been lose.\n", i);
	                                // Colse the connection in this case?
	                                close(pollarray[i].fd);
	                                pollarray[i].fd = -1;
                                    memset(computehost[i], 0, sizeof(computehost[i]));
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

                                else if (type == 1){//range request
                                	                              	
                                    int threadNum;                                        
                                    int range = 75000;     
                                    // receive the number of threads in the requesting process                                 
                                    fastreadvoid(pollarray[i].fd,&threadNum);
                                    printf("The thread %d in host %s to be started\n", threadNum, computehost[i]);                                    
                                    
                                    type = 2;
                                    //send the initial starts and ranges
                                    
                                	pt = (node *) calloc(1, sizeof (node)); 
                                	fastwrite(pollarray[i].fd,&type);
                                	fastwrite(pollarray[i].fd,&threadNum);
                                    fastwrite(pollarray[i].fd,&lastStart);
                                    fastwrite(pollarray[i].fd,&range);
                                    printf("New start %d and range %d sent back to compute %s, thread %d.\n",lastStart, range, computehost[i], threadNum);

                                    pt-> start = lastStart; 
                                    pt-> range = range;
                                    strcpy(pt->Host, (char *)&computehost[i][0]);
                                    pt->next = head;
                                    // printf("pt->start = %d, pt->range = %d. \n", pt-> start, pt-> range);
                                    head = pt;
                                    lastStart = lastStart + range + 1;//always keep track of the last start;                                    
                                    
                                }
                                else if (type == 3){//range finished 
                                	// printf("Received a finished range.\n");
                                	//1. do 2 more receives: a. the start of the range, b. the time in seconds to finish the range
                                	int indx, sec, startDone, rangeDone, newRange;
                                	fastreadvoid(pollarray[i].fd,&indx);
                                	fastreadvoid(pollarray[i].fd,&startDone);
                                	fastreadvoid(pollarray[i].fd,&sec);
                                    printf("Thread %d of host %s finished the range starting at %d with %d seconds.\n", indx,computehost[i],startDone,sec);
                                	// printf("Finished reading from compute.\n");
                                	//2. remove the range finished from the linklist
                                	node *prev;
                                	pt = head;
                                	prev = NULL;
                                	while (pt!= NULL){
                                        // printf("pt->start = %d, pt->range = %d\n", pt->start, pt->range);
                                        if (pt->start == startDone) {  /* Found it. */
                                            rangeDone = pt->range;
                                            if (prev == NULL) {
                                                /* Fix beginning pointer. */
                                                head = pt->next;
                                            } else {
                                                /*
                                                 * Fix previous node's next to
                                                 * skip over the removed node.
                                                 */
                                                prev->next = pt->next;
                                            }

                                              /* Deallocate the node. */
                                            free(pt);
                                            pt = NULL;
                                        }
                                		else {
                                            prev = pt;
                                			pt = pt->next;                                		
                                		}
                                	}

                                	if(sec==0){
                                		// printf("Sec == %d, change to 1.\n",sec);
                                		sec=1;
                                		// printf("Sec == %d now.\n", sec);
                                	}	

                                	// printf("Seconds used for last calculation = %d\n", sec);
                                	//3. calculate the new range according to the last calculation time
                                	double times = ((double)15)/((double)sec);
                                	printf("15 seconds is %lf time of the last calculation.\n", times);
                                	newRange =(int)((double)rangeDone * times);
                                	int times2 = (newRange*sec/rangeDone);
                                	printf("Estimated calculation time for new range %d is %d seconds.\n",newRange, times2);

                                	//4. send the new range
                                	type = 2;
                                	fastwrite(pollarray[i].fd,&type);
                                	fastwrite(pollarray[i].fd,&indx);
                                	fastwrite(pollarray[i].fd,&lastStart);
                                    fastwrite(pollarray[i].fd,&newRange);
                                    //store the new range to the linked list
                                    pt = (node *) calloc(1, sizeof (node)); 
                                    pt-> start = lastStart; 
                                    pt-> range = newRange;
                                    strcpy(pt->Host, (char *)&computehost[i][0]);
                                    pt->next = head;
                                    // printf("Add: pt->start = %d, pt->range = %d. \n", pt-> start, pt-> range);
                                    head = pt;
                                    printf("New range with start %d and range %d was sent back.\n",lastStart,newRange);
                                    lastStart = lastStart + newRange + 1;

                                }
                                else if (type == 4){//worker's currents
                                    // printf("Entered type = 4, reporting computing host %d 's currents\n", i);
                                	int curThreadNum, tested, curStart, end;
//TODO could have error on the number of chars sent
                                	write(reportfd,(computehost+i), 160*sizeof(char));//first write host name
                                    printf("Host name %s sent.\n", computehost[i]);
                                	fastreadvoid(pollarray[i].fd,&curThreadNum);
                                	fastwrite(reportfd,&curThreadNum);
                                    printf("Sent the number of threads = %d in this host \n", curThreadNum);
                                	int j;
                                	for(j=0;j<curThreadNum;j++){
                                		fastreadvoid(pollarray[i].fd,&tested);//tested 
                                		fastwrite(reportfd,&tested);
                                		fastreadvoid(pollarray[i].fd,&curStart);//curStart
                                		fastwrite(reportfd,&curStart);
                                		fastreadvoid(pollarray[i].fd,&end);//end
                                		fastwrite(reportfd,&end);
                                        // printf("In the %d th thread of this host, tested=%d, current=%d, end=%d.\n",j,tested,curStart,end);
                                	}
                                    printf("Report of each thread in host %s sent.\n", computehost[i]);
                                }                                
                                else if (type == 5){//perfect numbers
                                	//print the perfect number received (and what else need to be printed)?
                                	fastreadvoid(pollarray[i].fd,&(PerfRecords[TotalPerfNum].PerfNum));
                                	strcpy(PerfRecords[TotalPerfNum].PerfHost, (char *)&computehost[i][0]);
                                	printf("Perfect number %d found in host %s.\n",PerfRecords[TotalPerfNum].PerfNum, PerfRecords[TotalPerfNum].PerfHost);
                                	TotalPerfNum++;
                                }
                                else if (type == 6){//range terminated                                	
                                	//1. receive terminating range starters, remove the ranges from the linklist, print out the removed ranges
                                	int termStart, termRange, indx;
                                	fastreadvoid(pollarray[i].fd,&indx);
                                	fastreadvoid(pollarray[i].fd,&termStart);

                                    node *prev;
                                    pt = head;
                                    prev = NULL;
                                    while (pt!= NULL){
                                        // printf("pt->start = %d, pt->range = %d\n", pt->start, pt->range);
                                        if (pt->start == termStart) {  /* Found it. */
                                            termRange = pt->range;
                                            if (prev == NULL) {
                                                /* Fix beginning pointer. */
                                                head = pt->next;
                                            } else {
                                                /*
                                                 * Fix previous node's next to
                                                 * skip over the removed node.
                                                 */
                                                prev->next = pt->next;
                                            }

                                              /* Deallocate the node. */
                                            free(pt);
                                            // printf("Now the start stored in the prev of the linked list is:%d.\n", prev->start);
                                            pt = NULL;
                                        }
                                        else {
                                            prev = pt;
                                            pt = pt->next;                                      
                                        }
                                    }

                                  	printf("Thread %d of compute %s terminating.\n",indx, computehost[i]);                              	
                                	printf("Start %d and range %d revmoved from linked list.\n",termStart, termRange);
                                    // printf("Now the start stored in the head of the linked list is:%d.\n", head->start);
                                	type=11;
                                	fastwrite(pollarray[i].fd,&type);//send a confirm that range has been taken out from list

                                    // printf("termination = %d, (head == NULL) = %d\n", termination, (head==NULL));
                                	if((termination==1) && (head==NULL)){
                                		printf("All the thread in all computes removed from linked list, wait 5 seconds to close fds then terminate manage...\n");
                                		sleep(5);//make sure the last msg is sent
        		                        for (i=0;i<=19;i++) if (pollarray[i].fd != -1) {//close all
		                                	close(pollarray[i].fd);
			                                pollarray[i].fd = -1;
        		                        }
        		                        printf("All connections closed. Exit now.\n");
        		                        exit(0);
                                	}

                                	//3. close the socket for this compute:

                                }
                                else if (type == 8){//report's request
                                	reportfdindx = i;
                                	reportfd = pollarray[i].fd;
                                	compSent = 0;
                                	compNum = 0;
                                	int j;
                                	for (j=1;j<20;j++) {if((pollarray[j].fd!= -1) &&(j!=i))compNum++;}//number of computes
                                	printf("Received a report request. Currently %d compute hosts connected to manage.\n", compNum);
                                	type = 5;//perfect number
                                	fastwrite(reportfd,&type);
                                	fastwrite(reportfd,&compNum);
                                	fastwrite(reportfd,&TotalPerfNum);
                                	//first write the perfect numbers and the hostname who found them to report
                                	for (j=0;j<TotalPerfNum;j++){                                		
//TODO: this could be a problem                        		
                                		// fastwrite(reportfd,&(PerfRecords[j].PerfNum));
                                		write(reportfd,(PerfRecords+j), sizeof(PerfRecord));
                                		//how to send hostname to report?
                                	}
                                	//second ask all the computes to send their currents                              	
                                	for(j=1;j<20;j++){//all the connections >0 and <i should be computes 
                                        if ((pollarray[j].fd!= -1) &&(j!=i)){
                                        type = 7;//request for current;
                                        fastwrite(pollarray[j].fd, &type);//use MComm and mutex to receive this signal?                                            
                                        }
                                	}
                                }
                                else if (type == 10){//report's termination request
                                	termination = 1;
                                    printf("Received a termination request from report, initiate termination.\n");
                                	int j;
                                	for(j=1;j<reportfdindx;j++){//all the connections >0 and <i should be computes 
                                		if(pollarray[j].fd!= -1){
	                                		type = 9;//request for current;
                                			fastwrite(pollarray[j].fd, &type);//use MComm and mutex to receive this signal?
                                		}
                                	}
                                }                            
                        }
                        // else {
                        // 	printf("Connection on poll index %d has some other problem. Remove fd.\n", i);
                        // 	close(pollarray[i].fd);
                        //     pollarray[i].fd = -1;
                        //     computehost[i][0] = '\0';
                        //     memset(computehost[i], 0, sizeof(computehost[i]));
                        // }
                }
        }
}

