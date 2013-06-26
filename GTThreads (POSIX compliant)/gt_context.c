#include "gt_context.h"

ucontext_t scheduler_context;
ucontext_t main_context;
char scheduler_stack[STACKSIZE];
static bool allthreadsfinished = false;
void threadCtrl( void *func(), void *arg ) {

	#ifdef DEBUG
		printf("THREADCTRL\n\tactiveThreadCount %d\n", activeThreadCount);
	#endif

	threadList[currentThread].active = true;
	threadList[currentThread].returnval = func(arg);

	if( !threadList[currentThread].exited ){
		threadList[currentThread].finished = true;
		threadList[currentThread].active = false;
	}

#ifdef DEBUG
	printf("\tThread Ctrl swapping to scheduler\n");
#endif

	setcontext( &scheduler_context );
	return;
}

void init_thread_structs(){
	int i;

	for( i=0; i<MAXTHREADS; i++ ){
		threadList[i].active = false;
		threadList[i].finished = false;
		threadList[i].deleted = false;
		threadList[i].exited = false;
		threadList[i].threadid = -1;
	}
	return;
}

void make_sched_context(){

	if( !getcontext(&scheduler_context) ){
		scheduler_context.uc_stack.ss_sp = malloc( STACKSIZE );
		scheduler_context.uc_stack.ss_size = STACKSIZE;
		scheduler_context.uc_stack.ss_flags = 0;
		scheduler_context.uc_link = NULL;

#ifdef DEBUG
		printf("\n\tSched Context is %ld\n", scheduler_context.uc_stack.ss_sp );
#endif

		makecontext( &scheduler_context, (void(*)(void)) scheduler, 0 );
	}

	return;
}

void make_main_context(){

	if( !getcontext( &main_context ) ){
		main_context.uc_link = NULL;
		main_context.uc_stack.ss_sp = malloc(STACKSIZE);
		main_context.uc_stack.ss_size = STACKSIZE;
		main_context.uc_stack.ss_flags = 0;
	}

	if( main_context.uc_stack.ss_sp == 0 ){
		printf( "Error: Could not allocate stack" );
		exit(-1);
	}

#ifdef DEBUG
	printf("\n\tMain Context is %ld\n", main_context.uc_stack.ss_sp );
#endif

	return;
}

int make_thread_context( gtthread_t *thread, void *(*start_routine)(void *), void *arg ){
	gtthread_block_signal(SIGPROF);
	generate_thread_id( thread );

	getcontext( &threadList[threadCreateIndex].context );
	threadList[threadCreateIndex].threadid = *thread;
	threadList[threadCreateIndex].context.uc_link = &scheduler_context;
	threadList[threadCreateIndex].context.uc_stack.ss_sp = malloc(STACKSIZE);
	threadList[threadCreateIndex].context.uc_stack.ss_size = STACKSIZE;
	threadList[threadCreateIndex].context.uc_stack.ss_flags = 0;

	if( threadList[threadCreateIndex].context.uc_stack.ss_sp == 0 ) {
		printf( "Error: Could not allocate stack.");
		return -1;
	}

#ifdef DEBUG
	printf("\nMAKE THREAD CONTEXT:\n\t Allocating stack for thread %ld with context %ld\n", threadCreateIndex, threadList[threadCreateIndex].context.uc_stack.ss_sp );
#endif

	makecontext( &threadList[threadCreateIndex].context, (void(*)(void)) threadCtrl, 2, start_routine, arg );

	gtthread_unblock_signal(SIGPROF);
	return 0;
}

void generate_thread_id( gtthread_t *thread ){
	int i;
	static bool unique = false;
	do{
		*thread = rand(); //Generate new thread ID and assign.
		for( i=0; i<MAXTHREADS; i++ ){
			if( gtthread_equal(threadList[i].threadid, *thread ) ){
				break;
			}
			else unique = true;
		}
	}while(!unique);
	return;
}

void scheduler() {
	// allthreadsfinished is a global variable, which is set to 1, if the thread function has finished
	while( !allthreadsfinished ){

#ifdef DEBUG
			printf("SCHEDULER\n\tCurrent thread %d\n", currentThread);
#endif

		gtthread_block_signal(SIGPROF);
		if( numThreadsCreated != 0 ){
					atomic_increment( &currentThread );
					atomic_modulus( &currentThread, &numThreadsCreated );
		}

#ifdef DEBUG
			printf("\tCurrent thread after mod %d\n", currentThread);
			printf("\tallthreadsfinished %d\n", allthreadsfinished);
			printf("\tactiveThreads %d\n", activeThreadCount);
			printf("\tnumthreadscreated %d\n", numThreadsCreated);
			printf("\tcurrentThread %d\n", currentThread);
#endif

		if( activeThreadCount==0 )
			allthreadsfinished = true;


		if( !threadList[currentThread].finished && !threadList[currentThread].exited ){
			gtthread_unblock_signal(SIGPROF);
			swapcontext( &scheduler_context, &threadList[currentThread].context );
		}
		else{
			gtthread_cancel(threadList[currentThread].threadid);
			gtthread_unblock_signal(SIGPROF);
		}

	}

#ifdef DEBUG
	printf("All threads finished\n");
#endif
}
