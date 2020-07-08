#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "readerwriter.h"

//CONSTANTS Define number of readers and writers
#define READERS 5
#define READS 5
#define WRITERS 5
#define WRITES 5

//GLOBAL important shared variables
unsigned int shared = 0; //Can use unsigned here as the result should not be operated on in reverse
pthread_mutex_t sharedLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t rPhase = PTHREAD_COND_INITIALIZER;
pthread_cond_t wPhase = PTHREAD_COND_INITIALIZER;
//Represents readers that are waiting
int waitingR = 0;
//Represents readers that are active
int R =0;

//PROGRAM initialization and managements in main 
int main(int argc, char *argv[]){
	//Create lists for threads and their respective numbers
	int i;
	int readerN[READERS];
	int writerN[WRITERS];
	pthread_t readerID[READERS];
	pthread_t writerID[WRITERS];

	//Init random number generator for safety in read write begin operations
	srandom((unsigned int) time(NULL));

	//Create readers
	for(i = 0; i < READERS; i++){
		readerN[i] = i;
		pthread_create(&readerID[i], NULL, rMain, &readerN[i]);
	}

	//Create writers
	for(i = 0; i < WRITERS; i++){
		writerN[i] =i;
		pthread_create(&writerID[i], NULL, wMain, &writerN[i]);
	}
					
	// Wait on readers to finish
	for(i = 0; i < READERS; i++) {
		pthread_join(readerID[i], NULL);
	}

	// Wait on writers to finish
	for(i = 0; i < WRITERS; i++) {
		pthread_join(writerID[i], NULL);
	}

	//Terminate function
	return 0;
	
}

//Function containing reader logic
void *rMain(void *threadArg){
	//Establish reader ID, interator and number for readers
	int ID = *((int*) threadArg);
	int i = 0;
	int numR = 0;

	//Begin reader logic for each thread
	for(i = 0; i < READS; i++){
		//Randomize time of entrance so that all writes and reads are not simultaneous
		usleep(1000 * (random() % (READERS + WRITERS)));
	
		//Enter critical section when reading is required
		pthread_mutex_lock(&sharedLock);
		waitingR++;
		//If mutex is locked by a writer thread, reader must wait
		while(R == -1){
			pthread_cond_wait(&rPhase, &sharedLock);
		}
		//After mutex is opened update global variables for writers
		waitingR--;
		numR = ++R;
		pthread_mutex_unlock(&sharedLock);

		// Read data
	  	fprintf(stdout, "[r%d] reading %u  [readers: %2d]\n",ID,shared,numR);

		// Exit critical section after reading is completed
	  	pthread_mutex_lock(&sharedLock);
	  	R--;
		//If this is the last reader, signal the writer
	  	if (R == 0) {
	  		pthread_cond_signal(&wPhase);
	  	}	  	
	  	pthread_mutex_unlock(&sharedLock);
		
	}
	//Terminate the thread
	pthread_exit(0);
}

void *wMain(void *threadArg){
	//Establish ID, interator and number for writers
	int ID = *((int*) threadArg);
	int i = 0;
	int numR = 0;

	//Begin write process for each writer
	for(i = 0; i < WRITES; i++){
		//Randomize time of entrance so that all writes and reads are not simultaneous
		usleep(1000 * (random() % (READERS + WRITERS)));

		//Enter critical section when writing is required
		pthread_mutex_lock(&sharedLock);
		//If mutex is locked by any reader threads, writer must wait
		while(R != 0){
			pthread_cond_wait(&wPhase, &sharedLock);
		}
		//After mutex is opened to writer update global varaibles
		R = -1;
		numR = R;
		pthread_mutex_unlock(&sharedLock);

		// Write data
	  	fprintf(stdout, "[w%d] writing %u* [readers: %2d]\n",ID,++shared,numR);

		// Exit critical section after writing is completed
	  	pthread_mutex_lock(&sharedLock);
	  	R = 0;
		//If there are readers waiting, broadcast to readers, else signal the writer
	  	if (waitingR > 0) {
	  		pthread_cond_broadcast(&rPhase);
	  	}
	  	else {
	  		pthread_cond_signal(&wPhase);
	  	}
	  	pthread_mutex_unlock(&sharedLock);
	}

	pthread_exit(0);
}
