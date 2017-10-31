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

//gcc -Wall -Werror hw3.c -pthread
//./a.out testfiles00 5 hw3-output00.txt

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
	fflush(NULL);
}

void readFile(char* file) {

	printf("INSIDE READFILE()\n");
	fflush(NULL);

	char* buffer = NULL;
	FILE *fp = fopen(file, "r");
	if (fp != NULL) {
	    /* Go to the end of the file. */
	    if (fseek(fp, 0L, SEEK_END) == 0) {
	        /* Get the size of the file. */
	        long bufsize = ftell(fp);
	        if (bufsize == -1) { 
	        	perror("ftell() failed");
	        }

	        /* Allocate our buffer to that size. */
	        buffer = (char*)calloc(bufsize, sizeof(char));

	        /* Go back to the start of the file. */
	        if (fseek(fp, 0L, SEEK_SET) != 0) { 
	        	perror("fseek() failed");
	        }
	        /* Read the entire file into memory. */
	        size_t newLen = fread(buffer, sizeof(char), bufsize, fp);
	        if (newLen == 0) {
	            perror("fread() failed");
	        } 
	        else {
	            buffer[++newLen] = '\0'; /* Just to be safe. */
	            handleWords(buffer);
	        }
	    }
	    fclose(fp);
	}
	free(buffer);
}


// void critical_section() {

// }

//for each thread call, we need just the file name
void* threadCall(void* arg) {
	char* file = *(char**)arg;
	//free(arg);

	printf("THREAD %u: I'm going to send each file: %s to function readFile()\n", (unsigned int)pthread_self(), file);
	fflush(NULL);

	readFile(file);

	//return the TID value
	unsigned int* x = malloc(sizeof(unsigned int));
	*x = pthread_self();
	printf("THREAD %u: Exiting...\n", (unsigned int)pthread_self());
	pthread_exit(x);
}

void bufferRun(char** directory, unsigned int numFiles) {
	pthread_t tid[numFiles];
	int i, rc;

	for (i = 0; i < numFiles; i++) {
		rc = pthread_create( &tid[i], NULL, threadCall, (void*)&(directory[i]));
		if (rc != 0) {
			fprintf(stderr, "MAIN: Could not create thread\n");
			fflush(NULL);
		}
	}

	for (i = 0; i < numFiles; i++) {
		unsigned int* x;
		rc = pthread_join(tid[i], (void**)&x);

		if (rc != 0) {
			fprintf(stderr, "MAIN: Could not join thread\n");
			fflush(NULL);
		}

		printf("MAIN: Joined child thread %u, which returned %u\n", (unsigned int)tid[i], *x);
		fflush(NULL);
		free(x);
	}

	printf("MAIN: All threads terminated successfully.\n");
	fflush(NULL);
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
		fflush(NULL);
		i++;
	}

	bufferRun(directory, numFiles);

	return EXIT_SUCCESS;
}