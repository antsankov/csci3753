#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//right now this is hardcoded to 4, maybe find some way to check procs for e.c?
#define NUM_THREADS 4

int main(){
	printf("Hello, this is the template!\n");
	return 0;
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