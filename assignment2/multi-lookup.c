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
#define MAXARGS 127
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

pthread_cond_t empty,full;
pthread_mutex_t mutex_t,mutex;

char errorstr[SBUFSIZE];
//position in queue
int count = 0;
//boolean for if the job is finished.
int finished = 0;
//number of producer threads
int producer_threads = 1;

//declare a matrix of strings with a max of 1024 strings, and 200 chars max each 	
queue q;

void consume(FILE* outputfp, char hostname[]){
	//we can redef later.
	char firstipstr[INET6_ADDRSTRLEN];

	while(!queue_is_empty(&q)){
		if(dnslookup(hostname, firstipstr, sizeof(firstipstr))== UTIL_FAILURE){
			fprintf(stderr, "dnslookup error: %s\n", hostname);
			strncpy(firstipstr, "", sizeof(firstipstr));
	    }

		else{	
	    	/* Write to Output File */
	    	fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
		}
	}
}

//Every producer thread has its own producer 
/*This is the producer thread*/
void *producer(void *arg) {
	char hostname[SBUFSIZE];
	FILE* inputfp = arg;
	//as long as there are things to produce
	
	while(fscanf(inputfp, INPUTFS, hostname) > 0){
		pthread_mutex_lock(&mutex);
		//buffer is full, wait
		while (queue_is_full(&q)){
			pthread_cond_wait(&empty,&mutex);
		}
		//define this
		queue_push(&q, hostname);
		pthread_cond_signal(&full);
		pthread_mutex_unlock(&mutex);
	}
	/* Close Input File */
	fclose(inputfp);	
	//nothing left in the thread file. Time to exit.
	pthread_exit(arg);
}

/*This is the consumer thread*/
void *consumer(void *arg)
{
	FILE* outputfp = arg;
	//as long as there are things to produce and something to consume
	while(!finished || count > 0)
	{
		pthread_mutex_lock(&mutex);
		while(queue_is_empty(&q)){
			pthread_cond_wait(&full, &mutex);
		}
		void * hostname = queue_pop(&q);
		pthread_cond_signal(&empty);		
		pthread_mutex_unlock(&mutex);
		consume(outputfp, hostname);
	}
	pthread_exit(arg);
}

// /*This is the method that actually does work and is called by producer thread*/
// void produce(char[] hostname)
// {
// 		if(queue_push(&q, hostname) == QUEUE_FAILURE){
// 			sprintf(errorstr,"Couldn't push onto queue");
// 		}
// 		else {
// 			count++;
// 		}
// }


int main(int argc, char* argv[]){
	printf("Hello, this is the main!\n");
	FILE* outputfp = NULL;
	/* Check Arguments */
    if(argc < MINARGS){
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }

	//build the queue
	if(queue_init(&q, SBUFSIZE) == QUEUE_FAILURE){
		fprintf(stderr,"error: queue_init failed!\n");
    }
    //this is to the end of the input file index in the args
    int inputEnd = (argc-2);
	//create producer array
	pthread_t producers[MAXARGS];

	int i;
	/* Loop Through arguments for input files only!, creating a producer thread for each name file */
    for(i=1; i <= inputEnd; i++){
    	FILE* inputfp = NULL;
		inputfp = fopen(argv[i], "r");
		
		if(!inputfp){
		    sprintf(errorstr, "Error Opening Input File");
		    perror(errorstr);
		}
		pthread_create(&producers[i], NULL, producer, (void *) inputfp);
	    
	    //count 1+number of spawned requestors
	    producer_threads++;
    }

    //Open Output File
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
		perror("Error Opening Output File");
		return EXIT_FAILURE;
    }

    //Create NUM_THREADS consumers
    pthread_t consumers[NUM_THREADS];

    for(i=0; i < NUM_THREADS ; i++){
    	pthread_create(&consumers[i], NULL, consumer, (void *) outputfp);
    }

    //loop join on all the producer threads
    //note that i specifically starts at 1
    for(i=1; i < producer_threads; i++){
    	pthread_join(producers[i], NULL);
    }
    
    finished = 1; 

    //loop join on all the consumer threads
    for(i=0 ; i < NUM_THREADS; i++){
    	pthread_join(consumers[i], NULL);
    }

    /* Close Output File */
    fclose(outputfp);

    return EXIT_SUCCESS;
}

