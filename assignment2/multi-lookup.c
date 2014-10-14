#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "util.h"
#include "queue.h"

//right now this is hardcoded to 4, maybe find some way to check procs for e.c?
//#define NUMTHREADS 4
//sysconf(_SC_NPROCESSORS_ONLN);


#define MINARGS 3
#define MAXARGS 127
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"


typedef struct producer_args_s{
    queue* q;
    FILE* file;
    FILE* out;
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
char resolved[100][SBUFSIZE];

int numberResolved =0;

//returns the number of cores available on the cpu
int cores()
{
	return sysconf( _SC_NPROCESSORS_ONLN );
}

//Every producer thread has its own producer 
/*This is the produvcer thread*/
void *producer(void *arg) {
	char hostname[SBUFSIZE];
	producer_args* parameters = arg;
	FILE* inputfp = parameters->file;
	queue* q = parameters->q;
	FILE* outputfp = parameters->out;
	//as long as there are things to produce
	char requests[100][SBUFSIZE];
	int numberOfElements = 0;
	while(fscanf(inputfp, INPUTFS, hostname) > 0){
		
		pthread_mutex_lock(&mutex);
		
		//buffer is full, wait
		while (queue_is_full(parameters->q)){
			printf("%s\n","sleep");
			pthread_cond_wait(&empty,&mutex);
		}
		//define this
		printf("Producer thread hostname is: %s\n",hostname );
		strcpy(requests[numberOfElements], hostname);
		numberOfElements++;
		//printf("before push %i\n",q.rear);
		queue_push(parameters->q, hostname);
		//printf("AFter push %i\n",q.rear);
		pthread_cond_signal(&full);
		pthread_mutex_unlock(&mutex);
	}
	printf("%s\n","Producer finished");
	/* Close Input File */
	fclose(inputfp);	
	//nothing left in the thread file. Time to exit.
	



	int stopper = numberOfElements;
	while(1){
		if(stopper == 0){
			printf("STOPINGNOW\n");
			pthread_exit(arg);
			break;
		}
		printf("GOing to Sleep\n");
		usleep(250000);
		printf("Waking up\n");
		int x, j;
		//this loop goes through the requests
		for(j = 0; j < numberOfElements; j++){
			//this goes through resolved
			for(x = 0; x < numberResolved; x++){
				pthread_mutex_lock(&mutex);
				// printf("TRYINGTOWORK: j is =  %s, x is: %s\n",requests[j],resolved[x]);

				if(strcmp(requests[j], resolved[x]) == 0){
					stopper--;
					if(stopper == 0){
						printf("My request has been resolved: %s\n", resolved[x]);
						strcpy(resolved[x],"foo");
						pthread_mutex_unlock(&mutex);
						pthread_exit(arg);
						break;
					}
					printf("My request has been resolved: %s\n", resolved[x]);
					strcpy(resolved[x],"foo");
				}
				pthread_mutex_unlock(&mutex);
			}
		}
	}
	pthread_exit(arg);
}

/*This is the consumer thread*/
void *consumer(void *arg)
{
	char * hostname;
	char * clone;
	consumer_args* parameters = arg;
	FILE* outputfp = parameters->file;
	char firstipstr[INET6_ADDRSTRLEN];

	//as long as there are things to produce and something to consume
	//printf("CONSUMER THREAD!\n" );
	/* While The Queue Is Not Empty */
	while(!queue_is_empty(parameters->q) || !finished){
		clone = malloc(1025*sizeof(char));
		/* Lock The Queue So Only This Thread Can Access It */
    	pthread_mutex_lock(&mutex);

    	/* Get Hostname Off Queue */
    	hostname = queue_pop(parameters->q);
    	if(hostname == NULL){
    		pthread_mutex_unlock(&mutex);
    		free(clone);
    		usleep(100);
    		continue;
    	}
    	strcpy(clone, hostname);
    	//printf("Hostname is: %s\n",hostname );
    	pthread_mutex_unlock(&mutex);

	    	/* Unlock The Queue */

			
			//printf("Hostname / clone is: %s / %s \n",hostname, clone );
			
			/* Lookup hostname and get IP string */	
		    if(dnslookup(clone, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE){
				fprintf(stderr, "dnslookup error: %s\n", clone);
				strncpy(firstipstr, "", sizeof(firstipstr));
		    }

		    /* Lock Output File In Order To Write To It */
		    pthread_mutex_lock(&file_mutex);

		    /* Write to Output File */
		    fprintf(outputfp, "%s,%s\n", clone, firstipstr);
		    printf("lookup %s : %s\n",clone, firstipstr);
		    strcpy(resolved[numberResolved], clone);
		    numberResolved++;

		    /* Unlock Output File So Other Threads Can Write To It */
		    pthread_mutex_unlock(&file_mutex);
		
		/* Free Memory Blocks On The Heap Created By hostname */
		    free(clone);
	}
	printf("%s\n","consumer finished");
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

	
	FILE* outputfp = NULL;
	int numCPU = cores();
	printf("We are running with: %d cores\n", numCPU);
	
	//Declares an array of the two parameter structs
	producer_args arg_p[MAXARGS];
    consumer_args arg_c[numCPU];

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
		FILE* outfp = fopen(argv[argc-1], "r");
		//paramters
		arg_p[i].q = &q;
        arg_p[i].file = inputfp;
        arg_p[i].out = outfp;
		
		if(!inputfp){
		    sprintf(errorstr, "Error Opening Input File");
		    perror(errorstr);
		}
		pthread_create(&producers[i], NULL, producer, &arg_p[i]);
	    
	    //count 1+number of spawned requestors
	    producer_threads++;
    }

	//printf("Queue IS empty: %d\n",queue_is_empty(&q));

    //Open Output File
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
		perror("Error Opening Output File");
		return EXIT_FAILURE;
    }

    //Create numCPU consumers
    pthread_t consumers[numCPU];


    for(i=0; i < numCPU ; i++){
    	//paramters
		arg_c[i].q = &q;
        arg_c[i].file = outputfp;
    	pthread_create(&consumers[i], NULL, consumer, &arg_c[i]);
    }

        //kill the producers
    for(i=1; i < producer_threads; i++){
    	pthread_join(producers[i], NULL);
    }
    finished = 1;
    printf("###FINISHED PRODUCING###\n");

    for(i=0 ; i < numCPU; i++){
    	pthread_join(consumers[i], NULL);
    }

    /* Close Output File */
    fclose(outputfp);

    return EXIT_SUCCESS;
}

