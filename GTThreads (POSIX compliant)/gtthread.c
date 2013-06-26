#include "gtthread.h"

gtthread_t threadidarray[MAXTHREADS];
gtthread threadList[MAXTHREADS];

static bool initialized = false;

int currentThread = -1;
int numThreadsCreated = 0;
int threadCreateIndex = 0;
int activeThreadCount = 0;

void gtthread_init( long period ){
	//Set period to timequantum of scheduler, initialize thread control blocks (TCB)
	init_thread_structs();
	make_sched_context();
	init_itimerval( period );
	init_sched_preempt_handler();
	make_main_context();
	init_sigtimer();

	initialized = true;
	return;
}

int gtthread_create( gtthread_t *thread, void *(*start_routine)(void *), void *arg ){
	//Create thread
	if( !initialized ) {
		printf("Error: gtthread_init() not called! Exiting.");
		exit(-1);
	}

	if( numThreadsCreated >= MAXTHREADS )
		return -1; //Error value. Check pthread_create return value on error.

	int status = make_thread_context( thread, start_routine, arg );
	atomic_increment( &threadCreateIndex );
	atomic_increment( &numThreadsCreated );
	atomic_increment( &activeThreadCount );

	return status;
}

int gtthread_join( gtthread_t thread, void **status ){
	//Join thread
#ifdef DEBUG
	printf("JOIN:\n\tComing into join\n");
#endif

	int i;
	for( i=0; i<numThreadsCreated; i++ )
		if(threadList[i].threadid == thread) break;

	while( !threadList[i].exited && !threadList[i].finished ){
		gtthread_yield(); /*Just yield*/
	}

#ifdef DEBUG
	printf("JOIN:\n\tThread %d exited %d finished %d\n", i, threadList[i].exited, threadList[i].finished);
#endif

	if( ( threadList[i].exited || threadList[i].finished ) && status ){
		*status = (void *)threadList[i].returnval;
		return 0;
	}

return -1;
}

void gtthread_exit( void *retval ){
	//Terminate thread and set retval to join function
		threadList[currentThread].returnval = retval;
		threadList[currentThread].exited = true;
		gtthread_cancel( threadList[currentThread].threadid );

	setcontext( &scheduler_context );
	return;
}

void gtthread_yield( void ){
	//Suspend thread and yield CPU
	swapcontext( &threadList[currentThread].context, &scheduler_context);
	return;
}

int gtthread_equal( gtthread_t t1, gtthread_t t2 ){
	if( t1 == t2 ) return 1;
	else return 0;
}

int gtthread_cancel( gtthread_t thread ){
	//Cancel thread and release resources
	int i;
	for( i=0; i<numThreadsCreated; i++ ) {
		if( gtthread_equal( threadList[i].threadid, thread ) && !threadList[i].deleted ){
#ifdef DEBUG
			printf("CANCEL:\n\t Freeing thread %d with context %ld\n", i, threadList[i].context.uc_stack.ss_sp);
#endif
			free(threadList[i].context.uc_stack.ss_sp);
			threadList[i].active = false;
			threadidarray[i] = -1;
			atomic_decrement( &activeThreadCount );
			threadList[i].deleted = true;
#ifdef DEBUG
			printf("CANCEL:\n\tActive thread count decremented to %d\n", activeThreadCount);
			printf("\tThread %d cancelled\n", i);
#endif
		}
	}

	return 0;
}

gtthread_t gtthread_self( void ){
	return threadList[currentThread].threadid; //threadID
}


