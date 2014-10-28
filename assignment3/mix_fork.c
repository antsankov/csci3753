/*
 * File: pi.c
 * Author: Andy Sayler
 * Project: CSCI 3753 Programming Assignment 3
 * Create Date: 2012/03/07
 * Modify Date: 2012/03/09
 * Description:
 * 	This file contains a simple program for statistically
 *      calculating pi.
 */


//gcc  pi_fork.c -lm -o pi_fork
//DO WE NEED TO FORK or do we just spawn the program in a for loop in bash? 
/* Local Includes */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sched.h>

 /* Include Flags */
#define _GNU_SOURCE

/* RW System Includes */
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

///////////////RW STUFF////////////
#define MAXFILENAMELENGTH 80
#define DEFAULT_INPUTFILENAME "rwinput"
#define DEFAULT_OUTPUTFILENAMEBASE "rwoutput"
#define DEFAULT_BLOCKSIZE 1024
#define DEFAULT_TRANSFERSIZE 1024*100

////////////PI STUFF/////////////////
/* Local Defines */
#define DEFAULT_ITERATIONS 1000000
#define RADIUS (RAND_MAX / 2)


/* Local Functions */
inline double dist(double x0, double y0, double x1, double y1){
    return sqrt(pow((x1-x0),2) + pow((y1-y0),2));
}

inline double zeroDist(double x, double y){
    return dist(0, 0, x, y);
}


int main(int argc, char* argv[]){
    char *c = argv[argc - 1];
    int forks = atoi(c);
    int number, pid;
    int pids[forks];
    for (number=1;number<= forks;number++){
        pid = fork();
        if (pid == 0){
            printf("BREAKING OUT %d\n",number );
            break;
        }
        else
        {
            pids[number-1] = pid;
        }

    }

    if (pid == 0){
         /////////////PI PART////////////   
        int policy;
        struct sched_param param;

        int rv;
        int inputFD;
        int outputFD;
        char inputFilename[MAXFILENAMELENGTH];
        char outputFilename[MAXFILENAMELENGTH];
        char outputFilenameBase[MAXFILENAMELENGTH];

        ssize_t transfersize = 0;
        ssize_t blocksize = 0; 
        char* transferBuffer = NULL;
        ssize_t buffersize;

        ssize_t bytesRead = 0;
        ssize_t totalBytesRead = 0;
        int totalReads = 0;
        ssize_t bytesWritten = 0;
        ssize_t totalBytesWritten = 0;
        int totalWrites = 0;
        int inputFileResets = 0;
        
        //ADDED IN SCHEDULING!

        /* Set policy if supplied AT THE END OF THE PARAMETERS */
        if(!strcmp(argv[argc-2], "SCHED_OTHER")){
            policy = SCHED_OTHER;
        }
        else if(!strcmp(argv[argc-2], "SCHED_FIFO")){
            policy = SCHED_FIFO;
        }
        else if(!strcmp(argv[argc-2], "SCHED_RR")){
            policy = SCHED_RR;
        }
        else{
            fprintf(stderr, "Unhandeled scheduling policy\n");
            exit(EXIT_FAILURE);
        }

        
        /* Set process to max prioty for given scheduler */
        param.sched_priority = sched_get_priority_max(policy);
        
        /* Set new scheduler policy */
        fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
        fprintf(stdout, "Setting Scheduling Policy to: %d\n", policy);
        if(sched_setscheduler(0, policy, &param)){
        perror("Error setting scheduler policy");
        exit(EXIT_FAILURE);
        }
        fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));

        //ENDED SCHEDULING STUFF!



        /* Process program arguments to select run-time parameters */
        /* Set supplied transfer size or default if not supplied */
        if(argc < 2){
        transfersize = DEFAULT_TRANSFERSIZE;
        }
        else{
        transfersize = atol(argv[1]);
        if(transfersize < 1){
            fprintf(stderr, "Bad transfersize value\n");
            exit(EXIT_FAILURE);
        }
        }
        /* Set supplied block size or default if not supplied */
        if(argc < 3){
        blocksize = DEFAULT_BLOCKSIZE;
        }
        else{
        blocksize = atol(argv[2]);
        if(blocksize < 1){
            fprintf(stderr, "Bad blocksize value\n");
            exit(EXIT_FAILURE);
        }
        }
        /* Set supplied input filename or default if not supplied */
        if(argc < 4){
        if(strnlen(DEFAULT_INPUTFILENAME, MAXFILENAMELENGTH) >= MAXFILENAMELENGTH){
            fprintf(stderr, "Default input filename too long\n");
            exit(EXIT_FAILURE);
        }
        strncpy(inputFilename, DEFAULT_INPUTFILENAME, MAXFILENAMELENGTH);
        }
        else{
            if(strnlen(argv[3], MAXFILENAMELENGTH) >= MAXFILENAMELENGTH){
                fprintf(stderr, "Input filename too long\n");
                exit(EXIT_FAILURE);
            }
            strncpy(inputFilename, argv[3], MAXFILENAMELENGTH);
        }
        /* Set supplied output filename base or default if not supplied */
        if(argc < 5){
            if(strnlen(DEFAULT_OUTPUTFILENAMEBASE, MAXFILENAMELENGTH) >= MAXFILENAMELENGTH){
                fprintf(stderr, "Default output filename base too long\n");
                exit(EXIT_FAILURE);
            }
            strncpy(outputFilenameBase, DEFAULT_OUTPUTFILENAMEBASE, MAXFILENAMELENGTH);
        }
        else{
            if(strnlen(argv[4], MAXFILENAMELENGTH) >= MAXFILENAMELENGTH){
                fprintf(stderr, "Output filename base is too long\n");
                exit(EXIT_FAILURE);
            }
            strncpy(outputFilenameBase, argv[4], MAXFILENAMELENGTH);
        }

        /* Confirm blocksize is multiple of and less than transfersize*/
        if(blocksize > transfersize){
            fprintf(stderr, "blocksize can not exceed transfersize\n");
            exit(EXIT_FAILURE);
        }
        if(transfersize % blocksize){
        fprintf(stderr, "blocksize must be multiple of transfersize\n");
        exit(EXIT_FAILURE);
        }

        /* Allocate buffer space */
        buffersize = blocksize;
        if(!(transferBuffer = malloc(buffersize*sizeof(*transferBuffer)))){
        perror("Failed to allocate transfer buffer");
        exit(EXIT_FAILURE);
        }
        
        /* Open Input File Descriptor in Read Only mode */
        if((inputFD = open(inputFilename, O_RDONLY | O_SYNC)) < 0){
        perror("Failed to open input file");
        exit(EXIT_FAILURE);
        }

        /* Open Output File Descriptor in Write Only mode with standard permissions*/
        rv = snprintf(outputFilename, MAXFILENAMELENGTH, "%s-%d",
              outputFilenameBase, getpid());    
        if(rv > MAXFILENAMELENGTH){
        fprintf(stderr, "Output filenmae length exceeds limit of %d characters.\n",
            MAXFILENAMELENGTH);
        exit(EXIT_FAILURE);
        }
        else if(rv < 0){
        perror("Failed to generate output filename");
        exit(EXIT_FAILURE);
        }
        if((outputFD =
        open(outputFilename,
             O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
             S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) < 0){
        perror("Failed to open output file");
        exit(EXIT_FAILURE);
        }

        /* Print Status */
        fprintf(stdout, "Reading from %s and writing to %s\n",
            inputFilename, outputFilename);

        /* Read from input file and write to output file*/
        do{
        /* Read transfersize bytes from input file*/
        bytesRead = read(inputFD, transferBuffer, buffersize);
        if(bytesRead < 0){
            perror("Error reading input file");
            exit(EXIT_FAILURE);
        }
        else{
            totalBytesRead += bytesRead;
            totalReads++;
        }
        
        /* If all bytes were read, write to output file*/
        if(bytesRead == blocksize){
            bytesWritten = write(outputFD, transferBuffer, bytesRead);
            if(bytesWritten < 0){
            perror("Error writing output file");
            exit(EXIT_FAILURE);
            }
            else{
            totalBytesWritten += bytesWritten;
            totalWrites++;
            }
        }
        /* Otherwise assume we have reached the end of the input file and reset */
        else{
            if(lseek(inputFD, 0, SEEK_SET)){
            perror("Error resetting to beginning of file");
            exit(EXIT_FAILURE);
            }
            inputFileResets++;
        }
        
        }while(totalBytesWritten < transfersize);

        /* Output some possibly helpfull info to make it seem like we were doing stuff */
        fprintf(stdout, "Read:    %zd bytes in %d reads\n",
            totalBytesRead, totalReads);
        fprintf(stdout, "Written: %zd bytes in %d writes\n",
            totalBytesWritten, totalWrites);
        fprintf(stdout, "Read input file in %d pass%s\n",
            (inputFileResets + 1), (inputFileResets ? "es" : ""));
        fprintf(stdout, "Processed %zd bytes in blocks of %zd bytes\n",
            transfersize, blocksize);
        
        /* Free Buffer */
        free(transferBuffer);

        /* Close Output File Descriptor */
        if(close(outputFD)){
        perror("Failed to close output file");
        exit(EXIT_FAILURE);
        }

        /* Close Input File Descriptor */
        if(close(inputFD)){
        perror("Failed to close input file");
        exit(EXIT_FAILURE);
        }
        //////////////////////////////////////////////////////////
        long i;
        long iterations;
        double x, y;
        double inCircle = 0.0;
        double inSquare = 0.0;
        double pCircle = 0.0;
        double piCalc = 0.0;
        //Check for the for the 3 down from the end of the arguments
        iterations = atol(argv[argc-3]);
        /* Calculate pi using statistical methode across all iterations*/
        for(i=0; i<iterations; i++){
        x = (random() % (RADIUS * 2)) - RADIUS;
        y = (random() % (RADIUS * 2)) - RADIUS;
        if(zeroDist(x,y) < RADIUS){
            inCircle++;
        }
        inSquare++;
        }

        /* Finish calculation */
        pCircle = inCircle/inSquare;
        piCalc = pCircle * 4.0;

        /* Print result */
        fprintf(stdout, "pi = %f\n", piCalc);
    }

    else
    {
        int status;
        int i = 0;
        for(i = 0; i < forks ; i++)
        {
            pid = pids[i];
            do{
                pid = waitpid(pid, &status, WNOHANG);
                if (WIFEXITED(status))
                {
                    //printf("child exited with status of %d\n", WEXITSTATUS(status));
                }
            } while(pid == 0);
        }
    }
    return 0;
}

