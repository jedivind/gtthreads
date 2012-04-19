#define PHILOSOPHERS 5
#include <stdio.h>
#include <stdint.h>
#include "gtthread.h"
typedef struct philosopher{
gtthread_t philid;
}philosopher;

gtthread_mutex_t chopstick[PHILOSOPHERS];
philosopher phil[PHILOSOPHERS];

void *philfunc(void *phi)
{long i, pnum = (intptr_t)phi;
	while(1){
	printf("Philosopher %d thinking.", pnum);	
	for(i=0; i<rand()%1000000000; i++){/*Think*/ }
	gtthread_mutex_lock(&chopstick[pnum]);
	printf("Philosopher %d acquired left chopstick.\n", pnum);
	gtthread_mutex_lock(&chopstick[(pnum+1)%PHILOSOPHERS]);
	printf("Philosopher %d acquired right chopstick.\n", pnum);
	printf("Philosopher %d eating.\n", pnum);	
	for(i=0; i<rand()%100; i++){/*Eat*/}
	gtthread_mutex_unlock(&chopstick[pnum]);
	gtthread_mutex_unlock(&chopstick[pnum+1]);
	}
//printf("pnum = %d", pnum);
}

int main(int argc, char *argv[])
{int i;
	gtthread_init(500000);
	for(i=0; i<5; i++){int temp = i;
	gtthread_mutex_init(&chopstick[i]);
	gtthread_create(&phil[i].philid, philfunc, (void *)temp);
	}	
//for(;;);
gtthread_exit(NULL);
}
	


	
