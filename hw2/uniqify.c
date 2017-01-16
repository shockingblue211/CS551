// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Renjian Jiang
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #include <ctype.h>


int** creatFdTable(int** fdTable, int srtN){
	fdTable = malloc(srtN * sizeof(int*));
	int i;
	for (i = 0; i < srtN; i++)
		fdTable[i] = malloc(2 * sizeof(int));
	
	return fdTable;
}

char* lowerCase(char* str) {
	int i;
	for(i = 0;i < strlen(str);i++)
		str[i] = tolower(str[i]);
	
	return str;
}


void PnS (int** sorterfds, int** supprsefds, int srtN){

    pid_t cpid;

    int i;
    for (i = 0; i< srtN; i++){
        if (pipe(sorterfds[i]) == -1) {
           perror("pipe");
           exit(EXIT_FAILURE);
        }


        if (pipe(supprsefds[i]) == -1) {
           perror("pipe");
           exit(EXIT_FAILURE);
        }

        cpid = fork();
        if (cpid == -1) {
           perror("fork");
           exit(EXIT_FAILURE);
        }

        if (cpid == 0) {    /* Child reads from pipe */



	    // close former sorter and suppressor fds:
	    int k, j;

	    for (k = 0; k < i; k++){ 
	            // previous read side already closed in parent, fd of that previous pipe willl be assigned
	            //  to the read side of the current pipe, so if close the fd, acutally the current read fd is closed
	            // so only close the write side of the previous pipe, which is still open after forked from parent:
            close(sorterfds[k][1]);  
	        for (j = 0; j < 2; j++){
	           // both read the write side of the previous supp pipe are still opened after forked, so close both.
	            close(supprsefds[k][j]);
	        }
	    }
        close(sorterfds[i][1]);          /* Close unused write side */	        
        close(supprsefds[i][0]);		/* Close unused read side  */


        if (dup2(sorterfds[i][0], STDIN_FILENO) == -1)
         	printf("Stdin dup2 fail error at when i=%d...\n", i);

        if (dup2(supprsefds[i][1], STDOUT_FILENO) == -1)
          	printf("Stdout dup2 fail error...\n" );

        int exe;
        exe = execlp("/bin/sort", "/bin/sort", NULL);
        // shouldn't arrive here unless error:
        printf("%d\n", exe);

        _exit(EXIT_SUCCESS);

        } 
        else {            /* Parent writes argv[1] to pipe */
           close(sorterfds[i][0]);          /* Close unused read end */
        }
    }

}

void PnSP(int** sorterfds, int** supprsefds, int srtN, int status){

    pid_t cpid;
    char buf[50], buf2[40], buf3[40];
    FILE *bufchnl[srtN];
    FILE *bufchnl2[srtN];
    char* wordBuf[srtN];
    char* received;
    char smallest[42];
    int smallestcnt, locator, nullcnt;


    cpid = fork();
    if (cpid == -1) {
       perror("fork");
       exit(EXIT_FAILURE);
    }

    if (cpid == 0) {    /* Child reads from pipe */

	    // close sorter and suppressor fds:
	    int i, j;
	    for (i = 0; i < srtN; i++){
	       	// the read sides are already close  in parent, so only need and only can close the write side.
	    	close(sorterfds[i][1]);
	      	//Only suppressor write pipes can and need to closed:
	      	close(supprsefds[i][1]);
	    }

	 	// Set null count value:
	  	nullcnt = 0;

	    // Assign the first words in the buffers:
	  	for (i = 0; i < srtN; i++){
			bufchnl2[i] = fdopen (supprsefds[i][0] , "r");
			wordBuf[i] = malloc(43 * sizeof(char));
			received = fgets(wordBuf[i], 43, bufchnl2[i]);
			// printf("wordBuf[%d] is %s\n", i,  wordBuf[i]);
			if (received == NULL){
				wordBuf[i] = NULL;
				nullcnt++;
			} 
			// if (wordBuf[i] != NULL)
			// printf("First Loop: i = %d, Word: %s,string length: %lu\n", i, wordBuf[i],strlen(wordBuf[i]));

	  	}

	  	locator = 0;

	  	// search until all the wordBuf receive null:
	  	while(nullcnt<srtN){

	  		// rest the smallest word to the first non-null character:
	  		strncpy(smallest, wordBuf[locator],41);

	  		// printf("434343smallest: %s\n", smallest);
		  	smallestcnt = 0;
		 	// printf("Smallest: %s, word [%d]: %s\n", smallest, locator, wordBuf[i]);

	  		// printf("smallest: %s, locator = %d, srtN = %d\n", smallest, locator, srtN);

	  		// first loop to find the smallest number for the current while loop and its location:
		  	for (i = locator; i < srtN; i++){
			 	// printf("Smallest: %s, word [%d]: %s, compare: %d\n", smallest, i, wordBuf[i], strcmp(wordBuf[0], wordBuf[i]) );
		  		if (wordBuf[i] == NULL) {
		  			// printf("wordBuf[%d] == NULL, skip the current loop\n", i);
		  			continue;
		  		}

			 	// printf("Inside the first forloop: Smallest: %s, word [%d]: %s\n", smallest, i, wordBuf[i]);
			 	// printf("Smallest: %s, word [%d]: %s, compare: %d\n", smallest, i, wordBuf[i], strcmp(wordBuf[0], wordBuf[i]) );

		  		if (strcmp(smallest, wordBuf[i]) > 0){
		  			strncpy(smallest, wordBuf[i],41);
		  			locator = i;
		  		}		
		  	}

  		  	// starting at the current location: 
  		  	// 	1. continue reading until the next word is not the smallest word;
  		  	// 	2. find the next smallest in the next wordBuf;
  		  	// 	3. when this forloop exit, means all the current smallest word are read, 
  		  		// start next loop to find and read the next smallest word.
  		  	for (i = locator; i < srtN; i++){
			  		// printf("Now sorter at i = %d\n", i);
			  		// printf("The word at %d is %s\n", i, wordBuf[i]);
			  	if (wordBuf[i] == NULL) continue;	  		  		
			  	while(strcmp(smallest, wordBuf[i]) == 0){
			  		// printf("Go into the whileloop at i = %d\n", i);
  				  	// increase count of the current smallest word:
  		  			smallestcnt++;
				  	// get the next word from the smallest position:
				  	received = fgets(wordBuf[i], 43, bufchnl2[i]);
				  	// printf("wordBuf[%d] is %s\n", i,  wordBuf[i]);
				  	// printf("Received at i = %d is %s\n", i, received);
				  	// printf("smallest: %s wordBuf[%d]: %s\n",smallest, i, wordBuf[i] );
				  	// if received == NULL, set null and increase nullcnt:
				  	// if (*received == '\n') continue;
				  	if (received == NULL) {
					  	// printf("Received NULL at i= %d\n", i);	
				  		wordBuf[i] = NULL;
				  		nullcnt++;
				  		// printf("Now the nullcnt = %d\n", nullcnt);
				  		// stop searching since the pipe ends:
				  		break;
				  	}
				  	// else, if still the smallest word, increase smallestcnt, and continue reading:
				  	else if (strcmp(smallest, wordBuf[i]) == 0) {
				  		// printf("found another smallest\n");
				  		continue;
				  	}
				  	// else, the next must be larger than the smallest, since pipes are sorted. 
				  	// the next larger word is already stored in current wordBuf, 
				  	// just set locator back to 0 and stop searching:
				  	else {
				  		// printf("find a word larger than smallest\n");
				  		break;
				  	}
			  	}
  		  	}	//After this for loop, all the current smallest word has been counted, can print and find the count of 
  		  		// next smallest word in the next loop.

  		  	// print out:
  		  	// smallest[40]=0;
  		  	printf("%5d %s\n", smallestcnt, smallest);
  		  	// printf("Nullcnt = %d\n", nullcnt);

  		  	// set locator back to 0, then find the first non-null in the word buf:
  		  	locator = 0;
		  	for (i = 0; i < srtN; i++){
		  		if (wordBuf[i] == NULL) continue;
		  		else{
		  			// set the locator to the first non-null, then break:
		  			locator = i;
		  			break;
		  		}
 		  	}
	  	}

	  	// printf("Exited the suppressor loop, now close the pipes.\n");
	  	// close all the pipes, child program finish:
	  	for (i = 0; i < srtN; i++){
		    fflush(bufchnl2[i]);
		    close(supprsefds[i][0]);  
		    // printf("In Child, the i = %d pipes are closed\n", i);
	 	 }

	    exit(EXIT_SUCCESS);

    } 
    else {           

        // close suppressor fds:
        int i, j;
        for (i = 0; i < srtN; i++){
          	for (j = 0; j < 2; j++)
            	close(supprsefds[i][j]);
        }

      	// buffered write:

     	for (i = 0; i < srtN; i++)
          	bufchnl[i] = fdopen(sorterfds[i][1], "w");
     
      	//Scan and discard any leading unwanted chars
    	int retns = scanf("%*[^a-zA-Z]"); 
    	int RR = 0;
      	while (retns != EOF){
        	retns = scanf("%40[a-zA-Z]", buf);
        	// printf("buf = %s, buf lenght = %d\n", buf, strlen(buf) );

	        // Discard small strings:
	        if (strlen(buf)<3) {
		        buf[0] = '\0'; //"empty" buffer
		        retns = scanf("%*[^a-zA-Z]"); 
		        continue;
	        }

	        // // // Truncate >40 string:
	        // if (strlen(buf)>40){
	        // 	strncpy(buf3, buf, 40);
	        // 	buf3[40]=0;
	        // 	// printf("%s\n", buf3);
		       //  fputs(lowerCase(buf3), bufchnl[RR%srtN]);        	
		       //  // printf("%s\n", buf3);
		       //  fputs("\n", bufchnl[RR%srtN]);
	        // } 

	        // discard the emptry buffer
	        else if (buf[0] != '\0') { 
	        	buf[strlen(buf)+1]='\0';
	        	buf[strlen(buf)]='\n';

		        fputs(lowerCase(buf), bufchnl[RR%srtN]);
		        // printf("%s", buf);
		        // fputs("\n", bufchnl[RR%srtN]);
	        }
        
	        buf[0] = '\0'; //"empty" buffer
	        retns = scanf("%*[^a-zA-Z]"); 
	        RR++;
    	}
	      
	    for (i = 0; i < srtN; i++){
	         fclose(bufchnl[i]);
	         close(sorterfds[i][1]);          /* Reader will see EOF */
	         // printf("In parent, the i = %d pipe are closed\n", i);
	    }

	     /* Wait for child */
	    for (i = 0; i < (srtN+1); i++){
	    	wait(&status); 
	    	// printf("child i = %d returned\n", i);
	    }
	         

	    exit(EXIT_SUCCESS);
    }
}



int main(int argc, char const *argv[])
{	
	// number of sorters are specified in command line:
	// build a function that handles the number of sorters:
	if (argc<2){
		printf("Error, insufficient arguement.\n");
		exit(-1);
	}

	int srtN;
  	int status;
	// srtN = atoi(argv[1]);

	if ((srtN = atoi(argv[1]))== 0){
		printf("Error, wrong specification of number of sorters: %s\n", argv[1]);
		exit(-1);
	}

	int** sorterfds;
	int** supprsefds;

	sorterfds = creatFdTable(sorterfds, srtN);
	supprsefds = creatFdTable(supprsefds, srtN);

	PnS(sorterfds, supprsefds,srtN);
	PnSP(sorterfds, supprsefds,srtN,status);

	exit(EXIT_SUCCESS);	


// parser: 1. parse lines from stdin to words;
		// 3. parse all the words to lower cases;
		// 2. discard non-aphabetic words, discard words shorter then 3 charaters, truncate words over 40 characters
		// 4. round robin and assign words to sorters.

// sorter: sort the words using system sort command;

// supressor: 	1.sort the words come back from sorters;
			// 	2. provide counts and unique words.


}
