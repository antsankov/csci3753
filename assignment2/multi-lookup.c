#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "util.h"

//right now this is hardcoded to 4, maybe find some way to check procs for e.c?
#define NUM_THREADS 4

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"


condt empty,fill;
mutex_t mutex;  

//position in queue
int count = 0; 



//declare a matrix of strings with a max of 1024 strings, and 200 chars max each 	
char array[SBUFSIZE][255];

int main(int argc, char* argv[]){
	printf("Hello, this is the main!\n");

	int test;
	for (test = 1; test < argc; ++test){
		printf("Arg is: %s \n",argv[test]);
	}

    /* Check Arguments */
    if(argc < MINARGS){
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }
}

void *producer(void *arg) {
	int loops = (int) arg;
	int i; 
	for (i = 0; i < loops; i++){
		pthread_mutex_lock(&mutex);
		while (count == SBUFSIZE){
			pthread_cond_wait(&empty,&mutex);
		}
		//define this
		put(i);
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&mutex);

	}
}

void *consumer(void *arg)
{
	int i;
	int loops (int) arg;
	for(i = 0; i < loops; i++)
	{
		pthread_mutex_lock(&mutex);
		while(count == 0){
			pthread_cond_wait(&full, &mutex);
		}
		//define this 
		get(i);
		pthread_cond_signal(&empty);		
		pthread_mutex_unlock(&mutex);
	}
}

void lookup(char arr[]){

 /* Local Vars */
    FILE* inputfp = NULL;
    FILE* outputfp = NULL;
    char hostname[SBUFSIZE];
    char errorstr[SBUFSIZE];
    char firstipstr[INET6_ADDRSTRLEN];  

    /* Open Output File */
    outputfp = fopen(outputParam, "w");
    if(!outputfp){
		perror("Error Opening Output File");
		return EXIT_FAILURE;
    }

	inputfp = fopen(inputParam, "r");
	
	if(!inputfp){
	    sprintf(errorstr, "Error Opening Input File");
	    perror(errorstr);
	}
	

	/* Read File and Process*/
	while(fscanf(inputfp, INPUTFS, hostname) > 0){
	
	    
		//this is where we will be making the jobs 
	    /* Lookup hostname and get IP string */
	    if(dnslookup(hostname, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE){
			fprintf(stderr, "dnslookup error: %s\n", hostname);
			strncpy(firstipstr, "", sizeof(firstipstr));
	    }
		else{	
	    	/* Write to Output File */
	    	fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
		}
	}

	/* Close Input File */
	fclose(inputfp);
    /* Close Output File */
    fclose(outputfp);
}

//Rahat producer code:
/*
P{Produce();
	wait(empty);
	lock(lock);
	===========
	unlock(locl)
	post(full)
}

e{
	wait(full)
	lock*lock
	consume()
	unlock(lock)
	post(empty)
}



MultProdCon.c:

sem_t empty = N;
sem_t full = 0;
mutex_t lock;

*/