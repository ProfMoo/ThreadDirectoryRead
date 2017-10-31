#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>

#include "hw3.h"

//make a STRUCT here???

void* threadCall(void* arg);

void getFiles(char*** directory, char* location, int* numFiles) {
	DIR* dir = opendir(location);
	struct dirent* file;

	if (dir == NULL) {
		perror( "opendir() failed" );
	}

	int ret;
	ret = chdir(location);

	if (ret == -1) {
		perror( "chdir() failed" );
	}

	int i = 0;
	while((file = readdir(dir)) != NULL) {
		#if DEBUG_MODE
			printf( "found %s", file->d_name );
			fflush(NULL);
		#endif

		struct stat buf;

		int rc = lstat( file -> d_name, &buf );

		if(rc == -1) {
			perror("lstat() failed");
		}

		#if DEBUG_MODE
			printf( " (%d bytes)", (int)buf.st_size );
			fflush(NULL);
		#endif
		

		if (S_ISREG(buf.st_mode)) {
			(*numFiles)++;

			//adding to list of txt files here
			if (i == 0) {
				*directory = (char**)calloc(1, sizeof(char*));
			}
			else {
				*directory = realloc(*directory, (i+1)*sizeof(char*));
			}
			(*directory)[i] = (char*)calloc(80, sizeof(char));

			strcpy((*directory)[i], file->d_name);
			#if DEBUG_MODE
				printf( " -- regular file \n" );
				fflush(NULL);	
			#endif
			i += 1;
		}
		else if (S_ISDIR(buf.st_mode)) {
			#if DEBUG_MODE
				printf( " -- directory \n");
				fflush(NULL);
			#endif
		}
	}
	closedir(dir);
}

void handleWords(char* buff) {
	printf("buff: %s\n", buff);
}

void readFile(char* file) {
	int currentbuffsize = 0;
	char* totalBuffer;

	for( int i = 0; i < (*numFiles); i++ ) {
		char* buff = NULL;
		FILE* fp = fopen(file, "r");

		if (fp == NULL) {
			perror("ERROR: fopen() failed RIP");
			//break;
			continue;
		}
		if (fp != NULL) {
			#if DEBUG_MODE
				printf("reading %s\n", file);
			#endif
			//go to the end of the file	
			if (fseek(fp, 0L, SEEK_END) == 0) {
				long bufsize = ftell(fp); //get size of file
				if ( bufsize == -1 ) {
					perror( "ERROR: ftell() failed" );
				}

				if ( fseek(fp, 0L, SEEK_SET) != 0 ) {
					perror( "ERROR: fseek() failed" );
				}

				size_t newLen = fread(buff, sizeof(char), bufsize, fp); //read into memory
				if ( ferror(fp) != 0 ) {
					perror(" ERROR: fread() failed" );
				}
				else {
					buff[newLen++] = '\0';
				}
				handleWords(buff);
			}
			fclose(fp);
		}

		free(buff);
		buff = NULL;
	}
}

// void critical_section() {

// }

//for each thread call, we need just the file name
void threadCall(void* arg) {
	char* t = *(char**)arg;
	free(arg);

	printf("THREAD %d: I'm going to send each file to function readFile()", (int)pthread_self());

	readFile
}

void bufferRun(char** directory, int numFiles) {
	pthread_t tid[numFiles];
	int i, rc;

	int* t;

	for (i = 0; i < numFiles, i++) {
		rc = pthread_create( &tid[i], NULL, threadCall, &(directory[i]));
		if (rc != 0) {
			fprintf(stderr, "MAIN: Could not create thread\n");
		}
	}

	for (i = 0; i < numFiles, i++) {
		unsigned int* x;
		rc = pthread_join(tid[i], (void**)&x);

		if (rc != ) {
			fprintf(stderr, "MAIN: Could not join thread\n");
		}

		printf("MAIN: Joined child thread %u, which returned %u\n", (unsigned int)tid[i], *x);
		free(x);
	}

	printf("MAIN: All threads terminated successfully.\n");
	fflush(NULL);

	return EXIT_SUCCESS
}

int main(int argc, char* argv[]) {
	int i;

	//TO DO: have check for inputs here
	maxwords = atoi(argv[2]);
	words = (char**)calloc(maxwords, sizeof(char*));
	for(i = 0; i < maxwords; i++ ) {
		words[i] = (char*)calloc(80, sizeof(char));
	}

	#if DEBUG_MODE
		printf( "argv[0] is %s\n", argv[0]);
		printf( "argv[1] is %s\n", argv[1]);
		printf( "argv[2] is %s\n", argv[2]);
		printf( "argv[3] is %s\n", argv[3]);
	#endif

	int numFiles = 0;
	char** directory;
	//TO DO: DONT FORGET TO FREE THIS ^^^^
	getFiles(&directory, argv[1], &numFiles);
	i = 0;
	while (i < numFiles) {
		printf("files: %s\n", directory[i]);
		i++;
	}

	bufferRun(directory, numFiles);

	return EXIT_SUCCESS;
}