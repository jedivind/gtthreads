#include "gtthread_internal.h"

struct sigaction scheduling_preemption_handler;
struct itimerval timequant;

int initialized = 0, unique = 0;
static ucontext_t scheduler_context;
char scheduler_stack[STACKSIZE];

gtthread_t threadidarray[MAXTHREADS];
static gtthread threadList[MAXTHREADS];

static int currentthread = 0;
static int numthreads = 0;
static int threadnumber = 0;

static int allthreadsfinished = 0;
static int count = 0;
static int first = 1;

static void threadCtrl( void *func(), void *arg ) {
	threadList[currentthread].active = 1;
	threadList[currentthread].returnval = func(arg);	
	
	if( !threadList[currentthread].exited ){
		threadList[currentthread].finished=1;
		gtthread_cancel(threadList[currentthread].threadid);
	}
	
	setcontext( &scheduler_context );
	return;
}

static void mainCtrl() {
	threadList[0].active = 1;
	setcontext( &threadList[0].context );

	if( !threadList[0].exited ){
		threadList[0].finished=1;
		gtthread_cancel(threadList[0].threadid);
	}

	setcontext(&scheduler_context);
	return;
}

static void sighandler( int sig_nr, siginfo_t* info, void *old_context ) {

	if ( sig_nr == SIGPROF ) {
		swapcontext( &threadList[currentthread].context, &scheduler_context );
	}

	else return;
}

static void scheduler() {
	// allthreadsfinished is a global variable, which is set to 1, if the thread function has finished
	while( allthreadsfinished != 1 ){
		if( threadList[currentthread].finished != 1 && threadList[currentthread].exited != 1 ){
			swapcontext(&scheduler_context,&threadList[currentthread].context);
		}

		if( count == 1 && threadList[0].exited==1 ){ exit(0);}

		if( count==0 ){
			allthreadsfinished=1;
		}
		
		if( numthreads!=0 ){
			//printf("Current thread %d, finished %d exited %d\n", currentthread, threadList[currentthread].finished, threadList[currentthread].exited);
			currentthread=(currentthread+1)%numthreads;
		}
	}
}

void gtthread_init( long period ){
	//Set period to timequantum of scheduler
	int i;

	for( i=0; i<MAXTHREADS; i++ ){
		threadList[i].active = 0;
		threadList[i].finished = 0;
		threadList[i].deleted = 0;
		threadList[i].threadid = -1;
	}

	if( getcontext(&scheduler_context) == 0 ){
		scheduler_context.uc_stack.ss_sp = malloc(STACKSIZE);
		scheduler_context.uc_stack.ss_size = STACKSIZE;
		scheduler_context.uc_stack.ss_flags = 0;
		scheduler_context.uc_link = NULL; 
	}
	else {
		//printf("Error while initializing Scheduler Context!\n");
		exit(-1);
    }

	timequant.it_value.tv_usec = (long) period;
	timequant.it_interval = timequant.it_value;

	scheduling_preemption_handler.sa_sigaction = sighandler;
	scheduling_preemption_handler.sa_flags = SA_RESTART | SA_SIGINFO;
	sigemptyset(&scheduling_preemption_handler.sa_mask);
	if (sigaction(SIGPROF, &scheduling_preemption_handler, NULL) == -1) {
		printf("\nAn error occurred while initializing the signal handler for swapping to the scheduler context...\n");
		exit(-1);
	}

	initialized = 1;
	return;
}

int gtthread_create( gtthread_t *thread, void *(*start_routine)(void *), void*arg ){
	//Create thread
	int i;
	if( initialized != 1 ) {
		printf("Error: gtthread_init() not called! Exiting.");
		exit(-1);
	}

	if( numthreads >= MAXTHREADS ) return -1; //Error value. Check pthread_create return value on error.
	//Set thread ID
	do{
		*thread = rand(); //Generate new thread ID and assign.
		for( i = 0; i<MAXTHREADS; i++ ){
			if(threadList[i].threadid == *thread){
				break;
			}
			else unique = 1;
		}
	}while(!unique);

	getcontext( &threadList[threadnumber].context );
	if( threadnumber==0 ){
		// Set the context to a newly allocated stack
		threadList[0].threadid = 0;
		threadList[0].context.uc_link = &scheduler_context;
		threadList[0].context.uc_stack.ss_sp = malloc(STACKSIZE);
		threadList[0].context.uc_stack.ss_size = STACKSIZE;
		threadList[0].context.uc_stack.ss_flags = 0;
	
		if( threadList[0].context.uc_stack.ss_sp == 0 ){
			//printf( "Error: Could not allocate stack.", 0 );
			return -1;
	}
	
	if( setitimer(ITIMER_PROF, &timequant, NULL ) == 0) {
		//printf("The timer was initialized...\n");
	}
	else {
		printf("An error occurred while initializing timer. Please check the value of period to init()...\n");
		exit(-1);
	}
	
	// Create the context. The context calls threadCtrl( func ).
	makecontext( &threadList[0].context, (void(*)(void)) mainCtrl, 0, NULL, NULL );
	currentthread = threadnumber;
	numthreads++;
	count++;
	threadnumber++;	
	
	threadList[threadnumber].threadid = *thread;
	getcontext(&threadList[threadnumber].context);
	threadList[threadnumber].context.uc_link = &scheduler_context;
	threadList[threadnumber].context.uc_stack.ss_sp = malloc(STACKSIZE);
	threadList[threadnumber].context.uc_stack.ss_size = STACKSIZE;
	threadList[threadnumber].context.uc_stack.ss_flags = 0;	
	
	if( threadList[threadnumber].context.uc_stack.ss_sp == 0 ) {
		//printf( "Error: Could not allocate stack.", 0 );
		return -1;
	}
	makecontext( &threadList[threadnumber].context, (void(*)(void)) threadCtrl, 2, start_routine, arg );
	currentthread = threadnumber;
	numthreads++; 
	count++;
	threadnumber++;
	getcontext( &threadList[0].context );
	
	if( first ){
		first = 0;
		scheduler();
	}
	return 0;
	}

	else{
		threadList[threadnumber].threadid = *thread;
		threadList[threadnumber].context.uc_link = &scheduler_context;
		threadList[threadnumber].context.uc_stack.ss_sp = malloc(STACKSIZE);
		threadList[threadnumber].context.uc_stack.ss_size = STACKSIZE;
		threadList[threadnumber].context.uc_stack.ss_flags = 0;
	
	if ( threadList[threadnumber].context.uc_stack.ss_sp == 0 ){
		//printf( "Error: Could not allocate stack.", 0 );
		return -1;
	}

	makecontext( &threadList[threadnumber].context, (void(*)(void)) threadCtrl, 2, start_routine, arg);
	numthreads++;
	count++;
	threadnumber++; 
	swapcontext(&threadList[currentthread].context,&scheduler_context);
	return 0;
	}
}

int gtthread_join( gtthread_t thread, void **status ){
	//Join thread
	int i;
	for( i=0; i<numthreads; i++ )
		if(threadList[i].threadid == thread) break;

	while(threadList[i].exited != 1 && threadList[i].finished != 1){ /*Just wait*/ }

	if( ( threadList[i].exited || threadList[i].finished ) && status != NULL ){
		//printf("Assigning %s\n",threadList[i].returnval);
		*status = (void *)threadList[i].returnval;
		return 0;
	}
return -1;
}

void gtthread_exit( void *retval ){
	//Terminate thread and set retval to join function
	threadList[currentthread].returnval = retval;
	threadList[currentthread].exited = 1;

	if( threadList[currentthread].threadid != threadList[0].threadid )
		gtthread_cancel( threadList[currentthread].threadid );
	setcontext(&scheduler_context);
	return;
}

void gtthread_yield( void ){
	//Suspend thread and yield CPU
	int flag = 0;
	getcontext( &threadList[currentthread].context );

	if( flag == 0 ){
		flag = 1;
		setcontext( &scheduler_context );
	}
	return;
}

int gtthread_equal( gtthread_t t1, gtthread_t t2 ){
	if( t1 == t2 ) return 1;
	else return 0;
}

int gtthread_cancel( gtthread_t thread ){
	//Cancel thread and release resources
	int i;

	for( i=0; i<numthreads; i++ ) {
		if(threadList[i].threadid==thread && threadList[i].deleted != 1){
			free(threadList[i].context.uc_stack.ss_sp);
			threadList[i].active = 0;
			threadidarray[i] = -1;
			count--;
			threadList[i].deleted=1;
		}
	}
	return 0;
}

gtthread_t gtthread_self( void ){
	return threadList[currentthread].threadid; //threadID
}

int gtthread_mutex_init( gtthread_mutex_t *mutex ) {
	if( mutex->lock == 1 ) return -1;
	mutex->lock = 0;
	mutex->owner = -1;
	return 0;
}

int gtthread_mutex_lock( gtthread_mutex_t *mutex ){
	
	if( ( mutex->owner) == threadList[currentthread].threadid ){
		return -1;
	}
		
	while( mutex->lock !=0 && mutex->owner != threadList[currentthread].threadid )
	gtthread_yield();
		
	mutex->lock = 1;
	mutex->owner = threadList[currentthread].threadid;
	return 0;
}

int gtthread_mutex_unlock( gtthread_mutex_t *mutex ){
	if( mutex->lock == 1 && mutex->owner == threadList[currentthread].threadid ){
		mutex->lock = 0;
		mutex->owner = -1;
		return 0;
	}
	return -1;
}

