#define __GT_THREADS__
#define gtthread_t unsigned long int
#define STACKSIZE 65536
#define MAXTHREADS 20
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <malloc.h>

typedef int bool;
enum { false, true };

typedef struct gtthread{
	//Thread structure
	bool active;
	bool finished;
	bool exited;
	bool deleted;
	gtthread_t threadid;
	ucontext_t context;
	void *returnval;
}gtthread;

typedef struct gtthread_mutex_t{
	long lock;
	gtthread_t owner;
}gtthread_mutex_t;


void gtthread_init( long period );
int gtthread_create( gtthread_t *thread, void *(*start_routine)(void *), void *arg );
int gtthread_join( gtthread_t thread,void **status );
void gtthread_exit( void *retval );
void gtthread_yield( void );
int gtthread_equal( gtthread_t t1, gtthread_t t2 );
int gtthread_cancel( gtthread_t thread );
gtthread_t gtthread_self( void );

//Mutex operations
int  gtthread_mutex_init( gtthread_mutex_t *mutex );
int  gtthread_mutex_lock( gtthread_mutex_t *mutex );
int  gtthread_mutex_unlock( gtthread_mutex_t *mutex );

//Atomic operations
static void atomic_increment( int *operand );
static void atomic_decrement( int *operand );
static void atomic_modulus( int *operand, int *divisor );

//Internal operations
static void init_thread_structs();
static void init_itimerval( long period );
static void init_sigtimer();
static void init_sched_preempt_handler();

//Context operations
static void scheduler();
static void make_sched_context();
static void make_main_context();
static int make_thread_context( gtthread_t *thread, void *(*start_routine)(void *), void *arg );
static void generate_thread_id( gtthread_t *thread );

//Signal operations
void gtthread_block_signal(int signo);
void gtthread_unblock_signal(int signo);
