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


typedef struct gtthread {
	//Thread structure
	int active;
	int finished;
	int exited;
	int deleted;
	gtthread_t threadid;
	ucontext_t context;
	void *returnval;
}gtthread;

typedef struct gtthread_mutex_t {
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
int  gtthread_mutex_init( gtthread_mutex_t *mutex );
int  gtthread_mutex_lock( gtthread_mutex_t *mutex );
int  gtthread_mutex_unlock( gtthread_mutex_t *mutex );

