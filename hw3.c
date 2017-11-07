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

//function declaration
void* threadCall(void* arg);

pthread_mutex_t mutex_word = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_i = PTHREAD_MUTEX_INITIALIZER;

int current_index;
FILE* f;

/*===============================================================================================
<purpose> gets the textfile names from the directory
<input> a list of all the textfile names, directory, number of files
<output> 
===============================================================================================*/
void getFiles(char*** directory, char* location, int* numFiles) {
	DIR* dir = opendir(location);
	printf("MAIN: Opened \"%s\" directory\n", location);
	fflush(NULL);
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
		struct stat buf;

		int rc = lstat( file -> d_name, &buf );

		if(rc == -1) {
			perror("lstat() failed");
		}

		if (S_ISREG(buf.st_mode)) {
			(*numFiles)++;

			//adding to list of txt files here
			if (i == 0) {
				*directory = (char**)calloc(1, sizeof(char*));
			}
			else {
				*directory = realloc(*directory, (i+1)*sizeof(char*));
			}

			(*directory)[i] = (char*)calloc(80 + 1, sizeof(char));
			strncpy((*directory)[i], file->d_name, 80);

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
	printf("MAIN: Closed \"%s\" directory\n", location);
	fflush(NULL);
	closedir(dir);
}

/*================================================================================================
<purpose> clears the buffer
<input>
<output> 
===============================================================================================*/
void clearWordsCritical(void) {
	printf("MAIN: Buffer is full; writing %d words to output file\n", maxwords);
	fflush(NULL);
	int i = 0;
	while (i < maxwords) {
		fprintf(f, "%s\n", words[i]);
		words[i][0] = '\0';
		i += 1;
	}
}

/*================================================================================================
<purpose> function that writes to buffer. also checks
if the buffer needs to be added to the output file
<input> the word to be added, the index the word goes to
<output> 
===============================================================================================*/
void writeToWordsCritical(char* word, int i) {
	fflush(NULL);

	if (i == maxwords) { //if you need to write out to the file
		pthread_mutex_lock(&mutex_buffer);
		clearWordsCritical();
		i = 0;
		current_index = 0;
		pthread_mutex_unlock(&mutex_buffer);
	}
	//add word to words array
	pthread_mutex_lock(&mutex_buffer);

	strncpy(words[i], word, 80);

	printf("TID %u: Stored \"%s\" in shared buffer at index [%d]\n", (unsigned int)pthread_self(), word, i);
	fflush(NULL);
	
	pthread_mutex_unlock(&mutex_buffer); 
}

/*==============================================================================================
<purpose> begins mutex handling. sends each word to go
into the buffer, and locks/unlocks mutex
<input> each word to be added to buffer
<output> 
===============================================================================================*/
void mutexHandling(char* word) {
	pthread_mutex_lock(&mutex_word);
	writeToWordsCritical(word, current_index);
	current_index++;
	pthread_mutex_unlock(&mutex_word);
}

/*==============================================================================================
<purpose> parses through all the words in a textfile, 
sends each word to the next function
<input> the complete string from each file
<output> 
==============================================================================================*/
void handleWords(char* buff) {
	char* singleWord = (char*)calloc(80, sizeof(char));
	int i = 0;
	int j = 0;
	int wordCounter = -1;
	for(i = 0; i < strlen(buff); i++) {
		if (isalpha(buff[i]) || isdigit(buff[i])) { //begin word
			wordCounter += 1;
			j = 0;
			singleWord[0] = buff[i];
			while(1) { //reading a word
				j += 1;
				if(isalpha(buff[i+j])) {
					singleWord[j] = buff[i+j]; 
				}
				else if(isdigit(buff[i+j])) {
					singleWord[j] = buff[i+j]; 
				}
				else {
					break;
				}
			}
		}
		else {
			continue;
		}
		fflush(NULL);
		if (j > 1) {
			mutexHandling(singleWord); //BIG FUNCTION CALL
		}
		else {
			wordCounter -= 1;
		}
		memset(singleWord, 0, strlen(singleWord));
		i += j;
	}
	free(singleWord);
	singleWord = NULL;
}

/*===============================================================================================
<purpose> reads in the words from each textfile and 
send it to a new function
<input> the name for a single textfle
<output> 
===============================================================================================*/
void readFile(char* file) {
	char* buffer = NULL;
	FILE *fp = fopen(file, "r");
	printf("TID %u: Opened \"%s\"\n", (unsigned int)pthread_self(), file);
	pthread_mutex_unlock(&mutex_i);
	fflush(NULL);
	if (fp != NULL) {
	    /* Go to the end of the file. */
	    if (fseek(fp, 0L, SEEK_END) == 0) {
	        /* Get the size of the file. */
	        long bufsize = ftell(fp);
	        if (bufsize == -1) { 
	        	perror("ftell() failed");
	        }

	        /* Allocate our buffer to that size. */
	        buffer = (char*)calloc(bufsize+1, sizeof(char));

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
	            buffer[newLen] = '\0'; /* Just to be safe. */
	            handleWords(buffer); //BIG FUNCTION CALL
	        }
	    }
	    fclose(fp);
	    printf("TID %u: Closed \"%s\"; and exiting\n", (unsigned int)pthread_self(), file);
	    fflush(NULL);
	}
	free(buffer);
}

/*===============================================================================================
<purpose> each thread calls a function to read a file
<input> the void* pointer which contains a single textfile name
<output> 
===============================================================================================*/
void* threadCall(void* arg) {
	char* file = (char*)arg;

	readFile(file);

	//return the TID value
	unsigned int* x = malloc(sizeof(unsigned int));
	*x = pthread_self();
	pthread_exit(x);
}

/*===============================================================================================
<purpose> create threads and begin program purpose
<input> the array of text files, number of text files
<output> 
===============================================================================================*/
void bufferRun(char** directory, unsigned int numFiles) {
	pthread_t tid[numFiles];
	int i, rc;

	i = 0;
	while (i < numFiles) {
		pthread_mutex_lock(&mutex_i);
		rc = pthread_create( &tid[i], NULL, threadCall, (void*)(directory[i]));
		printf("MAIN: Created child thread for \"%s\"\n", directory[i]);
		fflush(NULL);
		if (rc != 0) {
			fprintf(stderr, "Could not create thread\n");
			fflush(NULL);
		}
		i++;
	}

	for (i = 0; i < numFiles; i++) {
		unsigned int* x;
		rc = pthread_join(tid[i], (void**)&x);

		if (rc != 0) {
			fprintf(stderr, "Could not join thread\n");
			fflush(NULL);
		}

		printf("MAIN: Joined child thread: %u\n", (unsigned int)tid[i]);
		fflush(NULL);
		free(x);
	}

	//catch any leftover words in the array that need to go to the file
	i = 0;
	while (i < current_index) {
		fprintf(f, "%s\n", words[i]);
		i += 1;
	}

	if (current_index == maxwords) {
		printf("MAIN: Buffer is full; writing %d words to output file\n", maxwords);
	}
	current_index = current_index%maxwords;
	printf("MAIN: All threads are done; writing %d words to output file\n", current_index);
	fflush(NULL);
}

int main(int argc, char* argv[]) {
	int i;

	//checking input args
	if (argc != 4) {
		fprintf(stderr, "ERROR: Invalid arguments\n");
		fprintf(stderr, "USAGE: ./a.out <input-directory> <buffer-size> <output-file>\n");
		return EXIT_FAILURE;
	}

	//dynamically allocate words array
	maxwords = atoi(argv[2]);
	words = (char**)calloc(maxwords, sizeof(char*));
	for(i = 0; i < maxwords; i++ ) {
		words[i] = (char*)calloc(80, sizeof(char));
	}
	printf("MAIN: Dynamically allocated memory to store %d words\n", maxwords);
	fflush(NULL);

	//open file
	f = fopen(argv[3], "w");
	if (f == NULL) {
	    printf("Error opening file!\n");
	    exit(1);
	}
	printf("MAIN: Created \"%s\" output file\n", argv[3]);
	fflush(NULL);

	//getting file and directory info
	int numFiles = 0;
	char** directory;
	getFiles(&directory, argv[1], &numFiles);

	//run actual reading and point of HW
	bufferRun(directory, numFiles);

	//free remaining words
	i = 0;
	while (i < maxwords) {
		free(words[i]);
		i += 1;
	}
	i = 0;
	while (i < numFiles) {
		free(directory[i]);
		i += 1;
	}
	free(words);
	free(directory);
	fclose(f);

	return EXIT_SUCCESS;
}