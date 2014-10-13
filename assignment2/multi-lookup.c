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

typedef struct producer_args_s{
    queue* q;
    FILE* file;
}producer_args;

typedef struct consumer_args_s{
    queue* q;
    FILE* file;
}consumer_args;






pthread_cond_t empty,full;
pthread_mutex_t mutex, file_mutex;


char errorstr[SBUFSIZE];
//position in queue
int count = 0;
//boolean for if the job is finished.
int finished = 0;
//number of producer threads
int producer_threads = 1;

//declare a matrix of strings with a max of 1024 strings, and 200 chars max each 	
queue q;

//Every producer thread has its own producer 
/*This is the produvcer thread*/
void *producer(void *arg) {
	char hostname[SBUFSIZE];
	producer_args* parameters = arg;
	FILE* inputfp = parameters->file;
	queue* q = parameters->q;
	//as long as there are things to produce
	
	while(fscanf(inputfp, INPUTFS, hostname) > 0){
		
		pthread_mutex_lock(&mutex);
		
		//buffer is full, wait
		while (queue_is_full(&q)){
			printf("%s\n","sleep");
			pthread_cond_wait(&empty,&mutex);
		}
		//define this
		printf("Producer thread hostname is: %s\n",hostname );
		//printf("before push %i\n",q.rear);
		queue_push(q, hostname);
		//printf("AFter push %i\n",q.rear);
		pthread_cond_signal(&full);
		pthread_mutex_unlock(&mutex);
	}
	printf("%s\n","Producer finished");
	/* Close Input File */
	fclose(inputfp);	
	//nothing left in the thread file. Time to exit.
	pthread_exit(arg);
}

/*This is the consumer thread*/
void *consumer(void *arg)
{
	char * hostname;
	char * clone;
	consumer_args* parameters = arg;
	FILE* outputfp = parameters->file;
	queue* q = parameters->q;
	char firstipstr[INET6_ADDRSTRLEN];

	//as long as there are things to produce and something to consume
	printf("CONSUMER THREAD!\n" );
	/* While The Queue Is Not Empty */
	while(!queue_is_empty(parameters->q) || !finished){

		/* Lock The Queue So Only This Thread Can Access It */
    	pthread_mutex_lock(&mutex);

    	/* Get Hostname Off Queue */
    	hostname = queue_pop(parameters->q);

    	if(hostname == NULL){
    		pthread_mutex_unlock(&mutex);	
    		usleep(100);
    	}
    	else {
	    	/* Unlock The Queue */
			pthread_mutex_unlock(&mutex);

			printf("before dns lookup %s\n",hostname);
			clone = malloc(1025*sizeof(char));
			strcpy(clone, hostname);
			printf("before clone %s\n",clone);
			/* Lookup hostname and get IP string */	
		    if(dnslookup(hostname, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE){
				fprintf(stderr, "dnslookup error: %s\n", hostname);
				strncpy(firstipstr, "", sizeof(firstipstr));
		    }
			printf("after clone %s\n",clone);
			printf("after dns lookup %s\n",hostname);

		    /* Lock Output File In Order To Write To It */
		    pthread_mutex_lock(&file_mutex);

		    /* Write to Output File */
		    fprintf(outputfp, "%s,%s\n", clone, firstipstr);

		    /* Unlock Output File So Other Threads Can Write To It */
		    pthread_mutex_unlock(&file_mutex);

		    /* Free Memory Blocks On The Heap Created By hostname */
		    free(clone);
		}
	}
	printf("%s\n","Were done in consumer" );
	pthread_exit(arg);
}

// void consume(FILE* outputfp, char hostname[]){
// 	//we can redef later.
// 	char firstipstr[INET6_ADDRSTRLEN];

// 	if(dnslookup(hostname, firstipstr, sizeof(firstipstr))== UTIL_FAILURE){
// 		fprintf(stderr, "dnslookup error: %s\n", hostname);
// 		strncpy(firstipstr, "", sizeof(firstipstr));
//     }
	
// 	else{
// 		copy = hostname;
// 		printf("Hostname in consume: %s\n", copy );	
// 		printf("hostname: %s,firstipstr: %s\n", copy, firstipstr);
//     	/* Write to Output File */
//     	fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
// 	}
// }

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

	printf("Hello, this is the main :)\n");
	pthread_mutex_t mutex;
	FILE* outputfp = NULL;
	
	//Declares an array of the two parameter structs
	producer_args arg_p[MAXARGS];
    consumer_args arg_c[NUM_THREADS];

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
	//initialize the mutex
	pthread_mutex_init(&mutex, NULL);

	int i;
	/* Loop Through arguments for input files only!, creating a producer thread for each name file */
    for(i=1; i <= inputEnd; i++){
    	FILE* inputfp = NULL;
		inputfp = fopen(argv[i], "r");
		//paramters
		arg_p[i].q = &q;
        arg_p[i].file = inputfp;
		
		if(!inputfp){
		    sprintf(errorstr, "Error Opening Input File");
		    perror(errorstr);
		}
		pthread_create(&producers[i], NULL, producer, &arg_p[i]);
	    
	    //count 1+number of spawned requestors
	    producer_threads++;
    }

	printf("qUEUE IS empty: %d\n",queue_is_empty(&q));

    //Open Output File
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
		perror("Error Opening Output File");
		return EXIT_FAILURE;
    }

    //Create NUM_THREADS consumers
    pthread_t consumers[NUM_THREADS];

    for(i=0; i < NUM_THREADS ; i++){
    	//paramters
		arg_c[i].q = &q;
        arg_c[i].file = outputfp;
    	pthread_create(&consumers[i], NULL, consumer, &arg_c[i]);
    }

    for(i=1; i < producer_threads; i++){
    	pthread_join(producers[i], NULL);
    }
    finished = 1;


    for(i=0 ; i < NUM_THREADS; i++){
    	pthread_join(consumers[i], NULL);
    }

    /* Close Output File */
    fclose(outputfp);

    return EXIT_SUCCESS;
}

