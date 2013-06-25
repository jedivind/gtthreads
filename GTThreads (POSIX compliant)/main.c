#include "gtthread.h"

gtthread_t threads[10];

void *thread_func( void *id ){
	printf("Hello from %ld thread\n", (intptr_t)id);
}

void main(){
	gtthread_init( 1000 );
	int i;
	for (i = 0; i < 10; i++)
    	        gtthread_create (&threads[i], thread_func, (void *)i);
  	    for (i = 0; i < 10; i++)
    	        gtthread_join (threads[i], NULL);
}

