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
    write(writefd,&netNum, sizeof(int));
}

typedef struct{
	int PerfNum;
	char PerfHost[160];
} PerfRecord;

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

int main(int argc, char const *argv[]){
	int type, sfd;
	type = 8;

//build up connection 
    struct sockaddr_in sin; /* socket address for destination */
    long address;
    int i,j;

    address = *(long *) gethostbyname(argv[1])->h_addr;
    sin.sin_addr.s_addr= address;
    sin.sin_family= AF_INET;
    sin.sin_port = htons(atoi(argv[2]));
    printf("Server hostname: %s and port number: %d has been added to address adder\n", argv[1], atoi(argv[2]));

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

    //sent request
    fastwrite(sfd, &type);
    //receive number of computes and perfect numbers first;
    int NComputes, Nperfect, computeUpdated,NThread,tested, curStart, end;
    char hostname[160];
    PerfRecord RecordRecv;
    computeUpdated = 0;
    fastreadvoid(sfd, &type);

    if(type == 5){//type 5 should have been received first, because it's in the same poll action of manage.
    	fastreadvoid(sfd, &NComputes);//knows how many computes are connnected
    	fastreadvoid(sfd, &Nperfect);//# of perfect numbers
    	printf("Currently %d compute host(s) found %d perfect numbers.\n",NComputes, Nperfect);
    	for(i=0;i<Nperfect;i++){
    		read(sfd,&RecordRecv,sizeof(PerfRecord));
    		printf("Perfect number %d found in host %s.\n", RecordRecv.PerfNum, RecordRecv.PerfHost);
    	}
    }

    for(i=0;i<NComputes;i++){//receive thread info
    	read(sfd,&hostname,160*sizeof(char));
    	fastreadvoid(sfd,&NThread);
    	printf("Compute on host %s has %d threads, report as below:\n", hostname, NThread);
    	memset(hostname, 0, sizeof(hostname));
    	for(j=0;j<NThread;j++){
    		fastreadvoid(sfd,&tested);
    		fastreadvoid(sfd,&curStart);
    		fastreadvoid(sfd,&end);
    		printf("Thread %d has tested %d numbers, its current range is %d to %d.\n", j, tested, curStart, end);  		
    	}
    }


    //if "-k":
    if (argc >3 && strcmp("-k", argv[3])  == 0){
		type = 10;
		printf("Client also wants to kill manage and computes.\n");
		fastwrite(sfd,&type);
	}
}
