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

int main(int argc, char* argv[]){
	printf("Hello, this is the template!\n");

	int test;
	for (test = 1; test < argc; ++test){
		printf("Arg is: %s \n",argv[test]);
	}
 /* Local Vars */
    FILE* inputfp = NULL;
    FILE* outputfp = NULL;
    char hostname[SBUFSIZE];
    char errorstr[SBUFSIZE];
    char firstipstr[INET6_ADDRSTRLEN];
    int i;
    
    /* Check Arguments */
    if(argc < MINARGS){
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
		perror("Error Opening Output File");
		return EXIT_FAILURE;
    }

    /* Loop Through Input Files */
    for(i=1; i<(argc-1); i++){
		/* Open Input File */
		inputfp = fopen(argv[i], "r");
		if(!inputfp){
		    sprintf(errorstr, "Error Opening Input File: %s", argv[i]);
		    perror(errorstr);
		    break;
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
    }

    /* Close Output File */
    fclose(outputfp);

    return EXIT_SUCCESS;
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