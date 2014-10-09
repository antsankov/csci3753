#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "queue.h"

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
//boolean for if the job is finished.
int finished = 0;



//declare a matrix of strings with a max of 1024 strings, and 200 chars max each 	
queue q;


int main(int argc, char* argv[]){
	printf("Hello, this is the main!\n");
	//build the queue
	if(queue_init(&q, SBUFSIZE) == QUEUE_FAILURE){
	fprintf(stderr,
		"error: queue_init failed!\n");
    }
    //
	int test;
	for (test = 1; test < argc; ++test){
		printf("Arg is: %s \n",argv[test]);
	}

/* Loop Through arguments */
    for(i=1; i<(argc-1); i++){
    	lookup(argv[i], argv[(argc-1)]);
    }



    /* Check Arguments */
    if(argc < MINARGS){
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }


}

void *producer(void *arg) {
	
	FILE* inputfp = *arg;
	//as long as there are things to produce
	while(!finished){
		pthread_mutex_lock(&mutex);
		//buffer is full, wait
		while (count == SBUFSIZE){
			pthread_cond_wait(&empty,&mutex);
		}
		//define this
		produce(inputfp);
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&mutex);

	}
}

void *consumer(void *arg)
{
	FILE* outputfp = *arg;
	//as long as there are things to produce and something to consume
	while(!finished || count > 0)
	{
		pthread_mutex_lock(&mutex);
		while(count == 0){
			pthread_cond_wait(&full, &mutex);
		}
		//define this 
		consume(outputfp);
		pthread_cond_signal(&empty);		
		pthread_mutex_unlock(&mutex);
	}
}

void produce(FILE* inputfp)
{
	char hostname[SBUFSIZE];
	if(fscanf(inputfp, INPUTFS, hostname) > 0)
	{
		array[count][0] = hostname;
		count++;
	}
	else
	{
		finished = 1;
	}
}

void consume(FILE* outputfp)
{

}


void lookup(int inputParam, int outputParam){

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
		//spawn a producer thread while theres a producer to spawn
		
	    /* Lookup hostname and get IP string */
	    if(dnslookup , firstipstr, sizeof(firstipstr)) == UTIL_FAILURE){
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