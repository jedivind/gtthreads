#ifndef __GT_THREADS__
#include "gtthread.h"
#endif

struct sigaction scheduling_preemption_handler;
struct itimerval timequant;

bool initialized = false, unique = false;
static ucontext_t scheduler_context;
static ucontext_t main_context;
char scheduler_stack[STACKSIZE];

gtthread_t threadidarray[MAXTHREADS];
static gtthread threadList[MAXTHREADS];

static int currentThread = -1;
static int numThreadsCreated = 0;
static int threadCreateIndex = 0;

static bool allthreadsfinished = false;
static int activeThreadCount = 0;

void gtthread_block_signal(int signo){
	sigset_t set;

	/* Block the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_BLOCK, &set, NULL);

return;
}

void gtthread_unblock_signal(int signo){
	sigset_t set;

	/* Block the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_BLOCK, &set, NULL);

return;
}

static void threadCtrl( void *func(), void *arg ) {
	//printf("threadCtrl activeThreadCount %d\n", activeThreadCount);
	threadList[currentThread].active = true;
	threadList[currentThread].returnval = func(arg);
	
	if( !threadList[currentThread].exited ){
		threadList[currentThread].finished = true;
		threadList[currentThread].active = false;
		gtthread_cancel(threadList[currentThread].threadid);
	}
	//printf("Thread Ctrl swapping to scheduler\n");
	setcontext( &scheduler_context );
	return;
}

static void atomic_increment( int *operand ){
	__asm__ __volatile__(
							"mov %0, %%eax\n\t"
							"add $0x01, %%eax\n\t"
							"xchg %%eax, %1\n\t"
							:"=r" (*operand)
							:"r" (*operand)
							:"%eax"
						);
	return;
}

static void atomic_decrement( int *operand ){
	__asm__ __volatile__(
							"mov %0, %%eax\n\t"
							"sub $0x01, %%eax\n\t"
							"xchg %%eax, %1\n\t"
							:"=r" (*operand)
							:"r" (*operand)
							:"%eax"
						);
	return;
}

static void atomic_modulus( int *operand, int *divisor ){

	__asm__ __volatile__(
							"mov %1, %%eax\n\t"
							"xor %%edx, %%edx\n\t"
							"mov %2, %%ecx\n\t"
							"idiv %%ecx\n\t"
							"mov %%edx, %0\n\t"
							:"=r"(*operand)
							:"r"(*operand), "r"(*divisor)
							:"%eax", "%ecx", "%edx"
						);
}

static void sighandler( int sig_nr, siginfo_t* info, void *old_context ) {

	if ( sig_nr == SIGPROF )
		swapcontext( &threadList[currentThread].context, &scheduler_context );

	else return;
}

static void init_thread_structs(){
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

static void init_itimerval( long period ){

	timequant.it_value.tv_usec = (long) period;
	timequant.it_interval = timequant.it_value;

}

static void init_sigtimer(){

	if( setitimer(ITIMER_PROF, &timequant, NULL ) == 0) {
		//printf("The timer was initialized...\n");
	}
	else {
		printf("An error occurred while initializing timer. Please check the value of period to init()...\n");
		exit(-1);
	}
	return;
}

static void init_sched_preempt_handler(){

	scheduling_preemption_handler.sa_sigaction = sighandler;
	scheduling_preemption_handler.sa_flags = SA_RESTART | SA_SIGINFO;
	sigemptyset(&scheduling_preemption_handler.sa_mask);

	if (sigaction(SIGPROF, &scheduling_preemption_handler, NULL) == -1) {
		printf("\nAn error occurred while initializing the signal handler for swapping to the scheduler context...\n");
		exit(-1);
	}
	return;
}

static void make_sched_context(){
	gtthread_block_signal(SIGPROF);
	if( !getcontext(&scheduler_context) ){
		scheduler_context.uc_stack.ss_sp = malloc( STACKSIZE );
		scheduler_context.uc_stack.ss_size = STACKSIZE;
		scheduler_context.uc_stack.ss_flags = 0;
		scheduler_context.uc_link = NULL;
		makecontext( &scheduler_context, (void(*)(void)) scheduler, 0 );
	}
	gtthread_unblock_signal(SIGPROF);
	return;
}

static void make_main_context(){
	gtthread_block_signal(SIGPROF);
	if( !getcontext( &main_context ) ){
		main_context.uc_link = NULL;
		main_context.uc_stack.ss_sp = malloc(STACKSIZE);
		main_context.uc_stack.ss_size = STACKSIZE;
		main_context.uc_stack.ss_flags = 0;
	}

	if( main_context.uc_stack.ss_sp == 0 ){
		printf( "Error: Could not allocate stack.", 0 );
		exit(-1);
	}
	gtthread_unblock_signal(SIGPROF);
	return;
}

static int make_thread_context( gtthread_t *thread, void *(*start_routine)(void *), void *arg ){
	gtthread_block_signal(SIGPROF);
	generate_thread_id( thread );

	getcontext( &threadList[threadCreateIndex].context );
	threadList[threadCreateIndex].threadid = *thread;
	threadList[threadCreateIndex].context.uc_link = &scheduler_context;
	threadList[threadCreateIndex].context.uc_stack.ss_sp = malloc(STACKSIZE);
	threadList[threadCreateIndex].context.uc_stack.ss_size = STACKSIZE;
	threadList[threadCreateIndex].context.uc_stack.ss_flags = 0;

	if( threadList[threadCreateIndex].context.uc_stack.ss_sp == 0 ) {
		printf( "Error: Could not allocate stack.", 0 );
		return -1;
	}
	makecontext( &threadList[threadCreateIndex].context, (void(*)(void)) threadCtrl, 2, start_routine, arg );
	//printf("Made context for thread %ld %ld\n", threadCreateIndex, *thread);
	gtthread_unblock_signal(SIGPROF);
	return 0;
}

static void generate_thread_id( gtthread_t *thread ){
	int i;
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

static void scheduler() {
	// allthreadsfinished is a global variable, which is set to 1, if the thread function has finished
	while( !allthreadsfinished ){//printf("Current thread %d\n", currentThread); //printf(" Thread %d finished %d exited %d\n", currentThread, threadList[currentThread].finished, threadList[currentThread].exited);
		gtthread_block_signal(SIGPROF);
		if( numThreadsCreated != 0 ){
					//printf("Current thread %d, finished %d exited %d\n", currentThread, threadList[currentThread].finished, threadList[currentThread].exited);
					atomic_increment( &currentThread );
					atomic_modulus( &currentThread, &numThreadsCreated );
					//printf("current thread after mod %d\n", currentThread);
					//printf(" allthreadsfinished %d\n", allthreadsfinished);
					//printf("active threads %d\n", activeThreadCount);
					//printf("numthreadscreated %d\n", numThreadsCreated);
					//printf("currentThread %d\n", currentThread);
		}

		if( activeThreadCount==0 ){
					allthreadsfinished = true; exit(0);
		}

		if( !threadList[currentThread].finished && !threadList[currentThread].exited ){
			gtthread_unblock_signal(SIGPROF);
			swapcontext( &scheduler_context, &threadList[currentThread].context );
		}


	}
	printf("All threads finished\n");
}

void gtthread_init( long period ){
	//Set period to timequantum of scheduler, initialize thread control blocks (TCB)
	gtthread_block_signal(SIGPROF);
	init_thread_structs();
	make_sched_context();
	init_itimerval( period );
	init_sched_preempt_handler();
	make_main_context();
	init_sigtimer();
	initialized = true;
	gtthread_unblock_signal(SIGPROF);
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
	//printf("Coming into join");
	int i;
	for( i=0; i<numThreadsCreated; i++ )
		if(threadList[i].threadid == thread) break;

	while( !threadList[i].exited && !threadList[i].finished ){ gtthread_yield();/*Just yield*/ }
	//printf("Thread %d exited %d finished %d\n",i, threadList[i].exited, threadList[i].finished);
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
		//printf("Coming into exit\n");
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
			free(threadList[i].context.uc_stack.ss_sp);
			threadList[i].active = false;
			threadidarray[i] = -1;
			atomic_decrement( &activeThreadCount );
			//printf("Active thread count decremented to %d\n", activeThreadCount);
			threadList[i].deleted = true;
			//printf("Thread %d cancelled\n", i);

		}
	}

	return 0;
}

gtthread_t gtthread_self( void ){
	return threadList[currentThread].threadid; //threadID
}

int gtthread_mutex_init( gtthread_mutex_t *mutex ) {
	if( mutex->lock == 1 ) return -1;

	__asm__ __volatile__ (
							"mov $0x00, %%rax\n\t"
							"xchg %%rax, %0\n\t"
							"mov $0xFFFFFFFFFFFFFFFF, %%rax\n\t"
							"xchg %%rax, %1\n\t"
							:"=r" (mutex->lock), "=r" (mutex->owner)
							:
							:"%rax"
						 );

	return 0;
}

int gtthread_mutex_lock( gtthread_mutex_t *mutex ){

	if( ( mutex->owner ) == threadList[currentThread].threadid ){
		return -1;
	}
		
	while( mutex->lock !=0 && mutex->owner != threadList[currentThread].threadid )
	gtthread_yield();

	__asm__ __volatile__(
						"mov $0x01, %%rax\n\t"
						"xchg %%rax, %0\n\t"
						"mov %2, %%rax\n\t"
						"xchg %%rax, %1\n\t"
						:"=r" (mutex->lock), "=r" (mutex->owner)
						:"r" (threadList[currentThread].threadid)
						:"%rax"
						);

	return 0;
}

int gtthread_mutex_unlock( gtthread_mutex_t *mutex ){

	if( mutex->lock == 1 && mutex->owner == threadList[currentThread].threadid ){

		__asm__ __volatile__ (
								"mov $0x00, %%rax\n\t"
								"xchg %%rax, %0\n\t"
								"mov $0xFFFFFFFFFFFFFFFF, %%rax\n\t"
								"xchg %%rax, %1\n\t"
								:"=r" (mutex->lock), "=r" (mutex->owner)
								 :
								 :"%rax"
							 );

		return 0;
	}
	return -1;
}

