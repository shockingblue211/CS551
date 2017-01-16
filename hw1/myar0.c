// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - RENJIAN JIANG
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ar.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <dirent.h> 

int main (int argc, char* argv[]){
	char prgmName[strlen(argv[0]-2)];
	strncpy(prgmName, argv[0]+2, strlen(argv[0]));
	printf("Program name: %s\n", prgmName);

	// At least 3 arguements to properly function:
	printf("number of args: %d\n", argc);//space after argv[2] will not be counted as argv[].
	

	char* arcfile;
	char * buff;
	int fd;
	arcfile = argv[2]; 
	buff =(char*) malloc(1000);
	struct ar_hdr archdstrc;
	int NOfFileInCommnd = argc - 3;
	printf("Number of files in command line: %d\n", NOfFileInCommnd);

	//First check existence of the archive file,
		// if don't exist: 
		if(open(arcfile,O_RDONLY) == -1){
			// q: create one;
			if (strcmp("q", argv[1])  == 0){
				// print the messgae like system ar does:
				printf("%s: creating %s\n", prgmName, argv[2]);


				// Like system ar, first check if all the files in the command exist in current directory:
				int dirfd [NOfFileInCommnd];
				// number of valid command line files:
				char validCommandFile = 0;
				for (int j = 3; j < argc; j++){
					// if file exist, open them and store their fds:
					if (open(argv[j],O_RDONLY) != -1){
						dirfd[j-3] = open(argv[j],O_RDONLY);	
						printf("fd number for argv[%d]: %d\n",j, dirfd[j-3]);
						validCommandFile++;
					}
					else{
						printf("%s: %s: No such file or directory\n", argv[1], argv[j]);
						break;								
					}
				}	

				printf("number of valid command files: %d\n", validCommandFile);
				//if the loop break in the middle:
				if (validCommandFile != NOfFileInCommnd) 
				{
					printf("invalid command file exist, abort session.\n");
					for (int i = 0; i < validCommandFile; i++)
					{
						// close the directory files which are openned:
						close(dirfd[i]);
					}
					// end session as error:
					exit(-1);
				}

				// else, if all the command files exist in directory, create a new archive and read them into it:
				umask(000);
				fd = creat(arcfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
				printf("RJ: Archive file now really created, fd = %d \n", fd);
				ssize_t nread = 0 ;
				ssize_t pointerChecker = 0;
				// firstly write in the magic number:
				nread = write(fd, ARMAG, SARMAG);
				pointerChecker += nread;
				printf("pointer after writing magic number: %ld\n", pointerChecker);
					
				// then read all the valid command files into archive:
				for (int i = 0; i < validCommandFile; i++){
					printf("RJ: current directory file fd = %d \n", dirfd[i]);
					// directory files don't have header, read their configurations into a header structure and write into archive:
					struct stat fileStatus;
					lstat(argv[i+3], &fileStatus);

					sprintf(archdstrc.ar_name, "%s%s", argv[i+3], "/");
					sprintf(archdstrc.ar_name, "%-16s", archdstrc.ar_name);
					sprintf(archdstrc.ar_date, "%-12d", (int)fileStatus.st_mtime);
					sprintf(archdstrc.ar_uid, "%-6d", (int)fileStatus.st_uid);
					sprintf(archdstrc.ar_gid, "%-6d", (int)fileStatus.st_gid);
					sprintf(archdstrc.ar_mode, "%-8o", (int)fileStatus.st_mode);
					sprintf(archdstrc.ar_size, "%-10d", (int)fileStatus.st_size);
					// printf("before  ar_fmag: %s\n", archdstrc.ar_name);
					strncpy(archdstrc.ar_fmag, ARFMAG, sizeof(archdstrc.ar_fmag));
					// printf("after  ar_fmag:  %s\n", archdstrc.ar_name);
					// still need to write file magic number. so leave it there.

					// now write the header into the newly created document:
					nread = write(fd, &archdstrc, sizeof(archdstrc));
					pointerChecker += nread;					
					printf("pointer after writing file header: %ld\n", pointerChecker);
					printf("size of content in int: %d\n", atoi(archdstrc.ar_size));

					// then write the contents of the file:
					// since no header located after file contents like in archive, just write until no more read:
					while ((nread = read(dirfd[i], buff, sizeof(buff))) > 0){
						write(fd, buff, nread);
						pointerChecker += nread;
					} 
					printf("pointer after writing file content: %ld\n", pointerChecker);

					// if file size is odd, add one 0 byte:
					if (atoi(archdstrc.ar_size) % 2 != 0){
						char padding = 0;
						nread = write(fd, &padding, 1);
						pointerChecker += nread;
						printf("pointer after padding one 0 byte: %ld\n", pointerChecker);
					}

					// close current directory fd:
					close(dirfd[i]);

				}

				// close the archive fd after all files were read in:
				close(fd);		
			}

			else if(argc<3) {
				printf("%s: No error\n", prgmName);
				exit(-1);
			}
			// other command: report error. 
			else 
				{
				printf("%s: %s: No such file or directory\n", prgmName, argv[2]);
				exit(-1);
				}	
		}
		// if exist, check magic number;
		else{

			fd = open(arcfile,O_RDONLY);
			printf("File exists, fd = %d \n", fd);
			char magicn[8];
			ssize_t nread ;
			ssize_t pointerChecker = 0;
			nread = read(fd, magicn, 8);
			pointerChecker += nread;
			printf("pointer after reading magic number: %ld\n", pointerChecker);
			// remember to null terminate the end of the string	
			magicn[8] = 0;
			printf("Magic number string:%s \n", magicn);
			printf("%ld character read\n", nread);


			// if not an archive, report error.
			if( strcmp(magicn,ARMAG) != 0){
				printf("%s: %s: File format not recognized\n", prgmName, argv[2]);
				exit(-1);
			}
			

			// if an archive, proceed.	
			else 
				printf("Is an archive file, proceed with key feature\n");
				// start building features here

				// t feature:
				if (strcmp("t", argv[1])  == 0){

					// if no file is specified from command line:
					if (NOfFileInCommnd == 0){
						while(1){
							nread = read(fd, &archdstrc, sizeof(archdstrc));
							// printf("size of arch head: %d\n", nread);
							if (nread == 0) break;

						/*	since printf always print all the way to the end,
							need the strtok with delimiter to only print name.*/
							printf("%s\n", strtok(archdstrc.ar_name, "/"));
							// printf("size of content in int: %d\n", atoi(archdstrc.ar_size));
							if (atoi(archdstrc.ar_size) % 2 == 0)
								nread = lseek(fd, atoi(archdstrc.ar_size), SEEK_CUR);
							else
								nread = lseek(fd, atoi(archdstrc.ar_size)+1, SEEK_CUR);
							// printf("current position of pointer: %d\n", nread);
						}
						// close archive file:
						close(fd);
					}	
					
					// if some file is specified from command line:
					else {
						//marker array
						char fileMarker [NOfFileInCommnd];

						for (int i = 0; i < NOfFileInCommnd; i++)
						{
							fileMarker[i] = 0;//initialize all the biomarker to 0
						}

						while(1){
							nread = read(fd, &archdstrc, sizeof(archdstrc));
							// printf("size of arch head: %d\n", nread);
							if (nread == 0) break;

						
							for (int j = 3; j < argc; j++){
								//if same name found, same info from header:
								if (strcmp(strtok(archdstrc.ar_name, "/"),argv[j]) == 0){
									fileMarker[j-3] = 1;
								}
							}
							
						
							// printf("size of content in int: %d\n", atoi(archdstrc.ar_size));
							if (atoi(archdstrc.ar_size) % 2 == 0)
								nread = lseek(fd, atoi(archdstrc.ar_size), SEEK_CUR);
							else
								nread = lseek(fd, atoi(archdstrc.ar_size)+1, SEEK_CUR);
							// printf("current position of pointer: %d\n", nread);
						}


						for (int i = 0; i < NOfFileInCommnd; i++)
						{
							if (fileMarker[i] == 1)
								printf("%s\n", argv [i+3]);
							else
								printf("no entry %s in archive\n", argv [i+3]);

						}
						// close archive file:
						close(fd);
					}
				}


				// tv feature:
				else if (strcmp("tv", argv[1])  == 0){

					// if no file is specified from command line:
					if (NOfFileInCommnd == 0){
						while(1){
							nread = read(fd, &archdstrc, sizeof(archdstrc));
							// printf("size of arch head: %d\n", nread);
							if (nread == 0) break;

							//oct 644= decimal 420, print out in ld, decimal format, so 420.
							// printf("file mode in decimal: %d \n", strtol(archdstrc.ar_mode,NULL,8)); 
							// printf("file mode in octal: %o \n", strtol(archdstrc.ar_mode,NULL,8));

							// printf("%d\n", S_IRUSR);//basically a bit string which indicates binary string 100000000, usr reading access.

							// mode print:
							printf( (strtol(archdstrc.ar_mode, NULL, 8) & S_IRUSR) ? "r" : "-");
							printf( (strtol(archdstrc.ar_mode, NULL, 8) & S_IWUSR) ? "w" : "-");
							printf( (strtol(archdstrc.ar_mode, NULL, 8) & S_IXUSR) ? "x" : "-");
		    				printf( (strtol(archdstrc.ar_mode, NULL, 8) & S_IRGRP) ? "r" : "-");
		    				printf( (strtol(archdstrc.ar_mode, NULL, 8) & S_IWGRP) ? "w" : "-");
		    				printf( (strtol(archdstrc.ar_mode, NULL, 8) & S_IXGRP) ? "x" : "-");
		    				printf( (strtol(archdstrc.ar_mode, NULL, 8) & S_IROTH) ? "r" : "-");
		  				  	printf( (strtol(archdstrc.ar_mode, NULL, 8) & S_IWOTH) ? "w" : "-");
		  				  	printf( (strtol(archdstrc.ar_mode, NULL, 8) & S_IXOTH) ? "x" : "-");

		  				  	// owner id and group id:
		  				  	printf(" %d/%d%+7ld ", atoi(archdstrc.ar_uid), atoi(archdstrc.ar_gid),atol(archdstrc.ar_size));
		  									
		  					// printf("\n");
		  					// printf("Header time string: %s ", strtok(archdstrc.ar_date, " "));
		  					// printf("\n");

		  				  	// store ar_date into time_t format to be used by strftime:
							time_t time = atoi(archdstrc.ar_date);
							// printf("Time as integer: %ld\n", time);

							// create a string to store formated date:
							char time_buf[18];
							// convert and store in time_buf:
							strftime(time_buf, 18, "%b %d %H:%M %Y", localtime(&time));//carries a null character at end
							printf(" %s ", time_buf);
							//file name:
							printf(" %s\n", strtok(archdstrc.ar_name, "/"));

							// printf("size of content in int: %d\n", atoi(archdstrc.ar_size));
							if (atoi(archdstrc.ar_size) % 2 == 0)
								nread = lseek(fd, atoi(archdstrc.ar_size), SEEK_CUR);
							else
								nread = lseek(fd, atoi(archdstrc.ar_size)+1, SEEK_CUR);
							// printf("current position of pointer: %d\n", nread);

						}
					}

					// if some file is specified from command line:
					else {
						//marker array
						int fileMarker [NOfFileInCommnd];

						for (int i = 0; i < NOfFileInCommnd; i++)
						{
							fileMarker[i] = 0;//initialize all the biomarker to 0
						}

						// use a header array to store all the headers with matched name:
						struct ar_hdr matchedHeader[NOfFileInCommnd + 1]; 
						// so that the position inf matched hearder arrary is none-zero, postion can be use for condition judgement:
						int posInMHs = 1;

						while(1){
							nread = read(fd, &archdstrc, sizeof(archdstrc));
							// printf("size of arch head: %d\n", nread);
							if (nread == 0) break;

							for (int i = 0; i < NOfFileInCommnd; i++){
								if (fileMarker[i] == 0){
									if (strcmp(strtok(archdstrc.ar_name, "/"), argv[i+3])  == 0){
										fileMarker[i] = posInMHs;
										matchedHeader [posInMHs++] = archdstrc;	
										break;
									}
								}
								else
									continue;	
							}
												
							// printf("size of content in int: %d\n", atoi(archdstrc.ar_size));
							if (atoi(archdstrc.ar_size) % 2 == 0)
								nread = lseek(fd, atoi(archdstrc.ar_size), SEEK_CUR);
							else
								nread = lseek(fd, atoi(archdstrc.ar_size)+1, SEEK_CUR);
							// printf("current position of pointer: %d\n", nread);
						}

						// reset pointer position in the matched header array:
						
						for (int i = 0; i < NOfFileInCommnd; i++)
						{
							if (fileMarker[i] != 0 ){
								printf( (strtol(matchedHeader[fileMarker[i]].ar_mode, NULL, 8) & S_IRUSR) ? "r" : "-");
								printf( (strtol(matchedHeader[fileMarker[i]].ar_mode, NULL, 8) & S_IWUSR) ? "w" : "-");
								printf( (strtol(matchedHeader[fileMarker[i]].ar_mode, NULL, 8) & S_IXUSR) ? "x" : "-");
			    				printf( (strtol(matchedHeader[fileMarker[i]].ar_mode, NULL, 8) & S_IRGRP) ? "r" : "-");
			    				printf( (strtol(matchedHeader[fileMarker[i]].ar_mode, NULL, 8) & S_IWGRP) ? "w" : "-");
			    				printf( (strtol(matchedHeader[fileMarker[i]].ar_mode, NULL, 8) & S_IXGRP) ? "x" : "-");
			    				printf( (strtol(matchedHeader[fileMarker[i]].ar_mode, NULL, 8) & S_IROTH) ? "r" : "-");
			  				  	printf( (strtol(matchedHeader[fileMarker[i]].ar_mode, NULL, 8) & S_IWOTH) ? "w" : "-");
			  				  	printf( (strtol(matchedHeader[fileMarker[i]].ar_mode, NULL, 8) & S_IXOTH) ? "x" : "-");

			  				  	// owner id and group id:
			  				  	printf(" %d/%d%+7ld ", atoi(matchedHeader[fileMarker[i]].ar_uid), atoi(matchedHeader[fileMarker[i]].ar_gid),atol(matchedHeader[fileMarker[i]].ar_size));
			  									
			  					// printf("\n");
			  					// printf("Header time string: %s ", strtok(matchedHeader[fileMarker[i]].ar_date, " "));
			  					// printf("\n");

			  				  	// store ar_date into time_t format to be used by strftime:
								time_t time = atoi(matchedHeader[fileMarker[i]].ar_date);
								// printf("Time as integer: %ld\n", time);

								// create a string to store formated date:
								char time_buf[18];
								// convert and store in time_buf:
								strftime(time_buf, 18, "%b %d %H:%M %Y", localtime(&time));//carries a null character at end
								printf(" %s ", time_buf);
								//file name:
								printf(" %s\n", strtok(matchedHeader[fileMarker[i]].ar_name, "/"));
								
							}
								
							else
								printf("no entry %s in archive\n", argv[i+3]);

						}

						// close archive file:	
						close(fd);	
					}

				}

				else if ((strcmp("x", argv[1])  == 0)|(strcmp("xo", argv[1])  == 0)){

					if (NOfFileInCommnd == 0){
						int dirfd;

						struct utimbuf achedtime;
						struct stat fileStatus;
						while(1){
							nread = read(fd, &archdstrc, sizeof(archdstrc));
							pointerChecker += nread;
							if (nread == 0) break;
							printf("%s\n", strtok(archdstrc.ar_name, "/"));
							printf("size of content in int: %d\n", atoi(archdstrc.ar_size));
							dirfd = open(strtok(archdstrc.ar_name, "/"), O_WRONLY | O_CREAT | O_TRUNC);
							fchmod(dirfd, strtol(archdstrc.ar_mode, NULL, 8));
							fchown(dirfd, atoi(archdstrc.ar_uid), atoi(archdstrc.ar_gid));

							lstat(strtok(archdstrc.ar_name, "/"), &fileStatus);

							printf("pointer after reading file header: %ld\n", pointerChecker);

							// read the file content from arch, and write to file in directory:
							for(int i = 0; i < atoi(archdstrc.ar_size) / sizeof(buff); i++){
								nread = read(fd, buff, sizeof(buff));
								write(dirfd, buff, nread);
								pointerChecker += nread;
							}
							nread = read(fd, buff, atoi(archdstrc.ar_size) % sizeof(buff));
							write(dirfd, buff, nread);
							pointerChecker += nread;

							printf("pointer after reading file contents: %ld\n", pointerChecker);

							// realize xo feature:
							if (strcmp("xo", argv[1])  == 0) {
								achedtime.actime = atoi(archdstrc.ar_date);
								achedtime.modtime = atoi(archdstrc.ar_date);
								utime(strtok(archdstrc.ar_name, "/"), &achedtime);
							}

							// close the current writing file in the directory:
							close(dirfd);
	
							// skip padding 0 char if size is odd:
							if (atoi(archdstrc.ar_size) % 2 != 0){
								nread = lseek(fd, 1, SEEK_CUR);
								pointerChecker = nread;
							}
							
							printf("Pointer after lseek for odd size (if applicable): %ld\n", pointerChecker);
						}
						// close archive file:
						close(fd);
					}

					else {
						int dirfd;
						struct utimbuf achedtime;
						struct stat fileStatus;
						char fileMarker [NOfFileInCommnd];

						for (int i = 0; i < NOfFileInCommnd; i++)
						{
							fileMarker[i] = 0;//initialize all the biomarker to 0
						}

						while(1){
							nread = read(fd, &archdstrc, sizeof(archdstrc));
							pointerChecker += nread;
							if (nread == 0) break;
							printf("%s\n", strtok(archdstrc.ar_name, "/"));
							printf("size of content in int: %d\n", atoi(archdstrc.ar_size));

							int foundAnyMatch;
							for (int i = 0; i < NOfFileInCommnd; i++){

								foundAnyMatch = 0;
								// if file not extracted yet:
								if (fileMarker[i] == 0)	{

									// if the current name from command line match the name at current acrh pointer:
									if (strcmp(strtok(archdstrc.ar_name, "/"), argv[i+3])  == 0)
									{
										dirfd = open(strtok(archdstrc.ar_name, "/"), O_WRONLY | O_CREAT | O_TRUNC);
										fchmod(dirfd, strtol(archdstrc.ar_mode, NULL, 8));
										fchown(dirfd, atoi(archdstrc.ar_uid), atoi(archdstrc.ar_gid));

										lstat(strtok(archdstrc.ar_name, "/"), &fileStatus);

										printf("pointer after reading file header: %ld\n", pointerChecker);

										// read the file content from arch, and write to file in directory:
										for(int i = 0; i < atoi(archdstrc.ar_size) / sizeof(buff); i++){
											nread = read(fd, buff, sizeof(buff));
											write(dirfd, buff, nread);
											pointerChecker += nread;
										}
										nread = read(fd, buff, atoi(archdstrc.ar_size) % sizeof(buff));
										write(dirfd, buff, nread);
										pointerChecker += nread;

										printf("pointer after reading file contents: %ld\n", pointerChecker);

										// realize xo feature:
										if (strcmp("xo", argv[1])  == 0) {
											achedtime.actime = atoi(archdstrc.ar_date);
											achedtime.modtime = atoi(archdstrc.ar_date);
											utime(strtok(archdstrc.ar_name, "/"), &achedtime);
										}

										// close the current writing file in the directory:
										close(dirfd);
				
										// skip padding 0 char if size is odd:
										if (atoi(archdstrc.ar_size) % 2 != 0){
											nread = lseek(fd, 1, SEEK_CUR);
											pointerChecker = nread;
										}
										
										printf("Pointer after lseek for odd size (if applicable): %ld\n", pointerChecker);

										// swift the marker to 0 so not assessed in the future:
										fileMarker[i] = 1;
										// found match in this forloop:
										foundAnyMatch = 1;
										// the current search found a match, time to move on to the next file in arch:
										break;
									}


								}

								else
									continue;
							}

							if (foundAnyMatch == 0) 
							{
								if (atoi(archdstrc.ar_size) % 2 == 0)
									pointerChecker = lseek(fd, atoi(archdstrc.ar_size), SEEK_CUR);
								else
									pointerChecker = lseek(fd, atoi(archdstrc.ar_size)+1, SEEK_CUR);

								printf("No match found for the file in arch, skip it to next file at: %ld\n", pointerChecker);								
							}
							
						}
						// close archive file:
						close(fd);

						for (int i = 0; i < NOfFileInCommnd; i++)
						{
							
							if (fileMarker[i] == 0)
								printf("no entry %s in archive\n", argv[i+3]);
						}
					}

				}

				else if (strcmp("q", argv[1])  == 0){

					// if no file is specified from command line:
					if (NOfFileInCommnd == 0){
						printf("Message to be deleted: no file specified to be stored in archive\n");
						// close archive file:
						close(fd);
					}

					// if some file is specified from command line:
					else {
						// overwrite the fd with read and WRITE access:
						fd = open(arcfile,O_WRONLY);

						// Like system ar, first check if all the files in the command exist in current directory:
						int dirfd [NOfFileInCommnd];
						// number of valid command line files:
						char validCommandFile = 0;
						for (int j = 3; j < argc; j++){
							// if file exist, open them and store their fds:
							if (open(argv[j],O_RDONLY) != -1){
								dirfd[j-3] = open(argv[j],O_RDONLY);	
								printf("fd number for argv[%d]: %d\n",j, dirfd[j-3]);
								validCommandFile++;
							}
							else{
								printf("%s: %s: No such file or directory\n", argv[1], argv[j]);
								break;								
							}
						}	

						printf("number of valid command files: %d\n", validCommandFile);
						//if the loop break in the middle:
						if (validCommandFile != NOfFileInCommnd) 
						{
							printf("invalid command file exist, abort session.\n");
							for (int i = 0; i < validCommandFile; i++)
							{
								// close the directory files which are openned:
								close(dirfd[i]);
							}
							// end session as error:
							exit(-1);
						}

						//existing archive file already exist, sleek to the end:
						pointerChecker = lseek(fd, 0, SEEK_END);
						printf("pointer after skip to file end: %ld\n", pointerChecker);

							
						// then read all the valid command files into archive:
						for (int i = 0; i < validCommandFile; i++){
							printf("RJ: current directory file fd = %d \n", dirfd[i]);
							// directory files don't have header, read their configurations into a header structure and write into archive:
							struct stat fileStatus;
							lstat(argv[i+3], &fileStatus);

							sprintf(archdstrc.ar_name, "%s%s", argv[i+3], "/");
							sprintf(archdstrc.ar_name, "%-16s", archdstrc.ar_name);
							sprintf(archdstrc.ar_date, "%-12d", (int)fileStatus.st_mtime);
							sprintf(archdstrc.ar_uid, "%-6d", (int)fileStatus.st_uid);
							sprintf(archdstrc.ar_gid, "%-6d", (int)fileStatus.st_gid);
							sprintf(archdstrc.ar_mode, "%-8o", (int)fileStatus.st_mode);
							sprintf(archdstrc.ar_size, "%-10d", (int)fileStatus.st_size);
							// printf("before  ar_fmag: %s\n", archdstrc.ar_name);
							strncpy(archdstrc.ar_fmag, ARFMAG, sizeof(archdstrc.ar_fmag));
							printf("after  ar_fmag:  %s\n", archdstrc.ar_name);
							// still need to write file magic number. so leave it there.

							// now write the header into the newly created document:
							printf("size of file header: %lu\n", sizeof(archdstrc));
							nread = write(fd, &archdstrc, sizeof(archdstrc));
							pointerChecker += nread;					
							printf("pointer after writing file header: %ld\n", pointerChecker);
							printf("size of content in int: %d\n", atoi(archdstrc.ar_size));

							// then write the contents of the file:
							// since no header located after file contents like in archive, just write until no more read:
							while ((nread = read(dirfd[i], buff, sizeof(buff))) > 0){
								write(fd, buff, nread);
								pointerChecker += nread;
							} 
							printf("pointer after writing file content: %ld\n", pointerChecker);

							// if file size is odd, add one 0 byte:
							if (atoi(archdstrc.ar_size) % 2 != 0){
								char padding = 0;
								nread = write(fd, &padding, 1);
								pointerChecker += nread;
								printf("pointer after padding one 0 byte: %ld\n", pointerChecker);
							}

							// close current directory fd:
							close(dirfd[i]);

						}

						// close the archive fd after all files were read in:
						close(fd);	
					}

				}

				else if (strcmp("A", argv[1])  == 0){

					// no need to specified any directory file, and no need to check.

					// overwrite the fd with read and WRITE access:
					fd = open(arcfile,O_WRONLY);

					//existing archive file already exist, sleek to the end:
					pointerChecker = lseek(fd, 0, SEEK_END);
					printf("pointer after skip to file end: %ld\n", pointerChecker);

					DIR *curDir;
  					struct dirent *dir;
  					struct stat fileStatus;


				  	curDir = opendir(".");
				  	if (curDir){
				  		// then read all the valid command files into archive:
						while ((dir = readdir(curDir)) != NULL) {
							
      						 // to extract the info
							lstat(dir->d_name, &fileStatus);

							sprintf(archdstrc.ar_name, "%s%s", dir->d_name, "/");
							sprintf(archdstrc.ar_name, "%-16s", archdstrc.ar_name);
							sprintf(archdstrc.ar_date, "%-12d", (int)fileStatus.st_mtime);
							sprintf(archdstrc.ar_uid, "%-6d", (int)fileStatus.st_uid);
							sprintf(archdstrc.ar_gid, "%-6d", (int)fileStatus.st_gid);
							sprintf(archdstrc.ar_mode, "%-8o", (int)fileStatus.st_mode);
							sprintf(archdstrc.ar_size, "%-10d", (int)fileStatus.st_size);
							strncpy(archdstrc.ar_fmag, ARFMAG, sizeof(archdstrc.ar_fmag));
							printf("Header printout:  %s\n", archdstrc.ar_name);

							// first to make sure don't operate on the current archive file:
							if (dir->d_name[0]!='.' && strcmp(dir->d_name, arcfile) != 0 && S_ISDIR(fileStatus.st_mode) == 0 ) {

									// if this is a symbolic link file
									if (S_ISLNK(fileStatus.st_mode)){

										ssize_t nread2 = 0;
										char  buff2[1000];
										// read link:
										nread2 = readlink(dir->d_name, buff2, sizeof(buff2));
										printf("directory read in: %s\n", buff2);

										// overwrite the size:
										sprintf(archdstrc.ar_size, "%-10d", (int)nread2);
										strncpy(archdstrc.ar_fmag, ARFMAG, sizeof(archdstrc.ar_fmag));
										printf("Header printout after reassign size:  %s\n", archdstrc.ar_name);

										// write the header:
										printf("size of file header: %lu\n", sizeof(archdstrc));
										nread = write(fd, &archdstrc, sizeof(archdstrc));
										pointerChecker += nread;					
										printf("pointer after writing file header: %ld\n", pointerChecker);
										printf("size of content in int: %d\n", atoi(archdstrc.ar_size));

										// write the content:
										nread = write(fd, buff, nread2);
										pointerChecker += nread2;
										printf("number of bytes read in for the link: %ld\n", nread2);
										printf("pointer after reading the file: %ld\n", pointerChecker);

										// if file size is odd, add one 0 byte:
										if (nread2 % 2 != 0){
											char padding = 0;
											nread = write(fd, &padding, 1);
											pointerChecker += nread;
											printf("pointer after padding one 0 byte: %ld\n", pointerChecker);
									}
								}


								// if this file is regular, add it to the acrhive: 
								else if(S_ISREG(fileStatus.st_mode)) {
									int dirfd;
									dirfd = open(dir->d_name, O_RDONLY);
									char magicn[8];
									nread = read(dirfd, magicn, 8);
									magicn[8] = 0;
									// if an archive, jump to next loop.
									if( strcmp(magicn,ARMAG) == 0){
										printf("%s is an archive, won't store it in %s\n", dir->d_name, argv[2]);
										continue;
									}
									// else, read to archive:
									else{									
										// now write the header:
										printf("size of file header: %lu\n", sizeof(archdstrc));
										nread = write(fd, &archdstrc, sizeof(archdstrc));
										pointerChecker += nread;					
										printf("pointer after writing file header: %ld\n", pointerChecker);
										printf("size of content in int: %d\n", atoi(archdstrc.ar_size));

										// jump back to the start and read, and then write to the archive:
										lseek(dirfd, 0, SEEK_SET);
										// then write the contents of the file:
										// since no header located after file contents like in archive, just write until no more read:
										while ((nread = read(dirfd, buff, sizeof(buff))) > 0){
											write(fd, buff, nread);
											pointerChecker += nread;
										} 
										printf("pointer after writing file content: %ld\n", pointerChecker);

										// if file size is odd, add one 0 byte:
										if (atoi(archdstrc.ar_size) % 2 != 0){
											char padding = 0;
											nread = write(fd, &padding, 1);
											pointerChecker += nread;
											printf("number of bytes padded(WRITTEN): %ld\n", nread);
											printf("pointer after padding one 0 byte: %ld\n", pointerChecker);
										}

										// close current directory fd:
										close(dirfd);	
									}
								} 		

								
								

				      		}
						}				
				  	}
					// close the archive fd after all files were read in:
					close(fd);									
				}

				else if (strcmp("d", argv[1])  == 0){

					if (NOfFileInCommnd == 0){
						printf("no file specified to be deleted\n");
					}

					else {
						int dirfd;
						ssize_t pointerCheckerNew = 0;
						char fileMarker [NOfFileInCommnd];

						for (int i = 0; i < NOfFileInCommnd; i++)
						{
							fileMarker[i] = 0;//initialize all the biomarker to 0
						}

						unlink(arcfile);
						umask(000);
						dirfd = creat(arcfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
						nread = write(dirfd, ARMAG, SARMAG);
						pointerCheckerNew += nread;
						// printf("New arch's pointer after writing magic number: %ld\n", pointerChecker);

						while(1){
							nread = read(fd, &archdstrc, sizeof(archdstrc));
							pointerChecker += nread;
							if (nread == 0) break;
							printf("file name: %s\n", strtok(archdstrc.ar_name, "/"));
							printf("size of content in int: %d\n", atoi(archdstrc.ar_size));
							printf("Old arch's pointer after reading header: %ld\n", pointerChecker);

							char foundAnyMatch = 0;
							// if any match found, skip the current file:
							for (int i = 0; i < NOfFileInCommnd; i++){
								// if file not delete once:
								if (fileMarker[i] == 0)	{
									printf("command file name: %s\n", argv[i+3]);
									// if the current name from command line match the name at current acrh pointer, skip this file:
									if (strcmp(strtok(archdstrc.ar_name, "/"), argv[i+3])  == 0)
									{
										
										if (atoi(archdstrc.ar_size) % 2 == 0)
											pointerChecker = lseek(fd, atoi(archdstrc.ar_size), SEEK_CUR);
										else
											pointerChecker = lseek(fd, atoi(archdstrc.ar_size)+1, SEEK_CUR);
										printf("Match found, skip the old acrh pointer it to next file at: %ld\n", pointerChecker);	
				
										// swift the marker to 0 so not assessed in the future:
										fileMarker[i] = 1;
										foundAnyMatch = 1;
										// found match, break loop:
										break;
									}
								}
							}

							if (foundAnyMatch ==1) continue;
							// if no match found, write the hearder and content to new arch file:
							// write hearder:
							nread = write(dirfd, &archdstrc, sizeof(archdstrc));
							pointerCheckerNew += nread;
							printf("New arch's pointer after writing header: %ld\n", pointerCheckerNew);

							// write content:
							for(int i = 0; i < atoi(archdstrc.ar_size) / sizeof(buff); i++){
								nread = read(fd, buff, sizeof(buff));
								pointerChecker += nread;
								nread = write(dirfd, buff, nread);
								pointerCheckerNew += nread;
							}
							nread = read(fd, buff, atoi(archdstrc.ar_size) % sizeof(buff));
							pointerChecker += nread;
							nread = write(dirfd, buff, nread);
							pointerCheckerNew += nread;

							printf("Old arch's pointer position after reading content: %ld\n", pointerChecker);
							printf("New arch's pointer position after writing content: %ld\n", pointerCheckerNew);


							// skip padding 0 char if size is odd:
							if (atoi(archdstrc.ar_size) % 2 != 0){
								nread = lseek(fd, 1, SEEK_CUR);
								pointerChecker = nread;
								nread = lseek(dirfd, 1, SEEK_CUR);
								pointerCheckerNew = nread;
								printf("Pointer after lseek for odd size in old arch: %ld\n", pointerChecker);
								printf("Pointer after lseek for odd size in new arch: %ld\n", pointerCheckerNew);
							}						
						}
						// close old archive file:
						close(fd);
						// close new archive file:
						close(dirfd);

					}

				}

				else printf("%s: no operation specified\n", prgmName);

		}
}
