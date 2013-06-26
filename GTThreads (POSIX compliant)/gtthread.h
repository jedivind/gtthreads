#ifndef __GT_THREADS__
#define __GT_THREADS__

#define gtthread_t unsigned long int
#define STACKSIZE 65536
#define MAXTHREADS 20

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <malloc.h>

#include "gt_signals.h"
#include "gt_context.h"
#include "gt_atomic_ops.h"
#include "gt_mutex.h"


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

extern gtthread_t threadidarray[MAXTHREADS];
extern gtthread threadList[MAXTHREADS];
extern int currentThread;
extern int numThreadsCreated;
extern int threadCreateIndex;
extern int activeThreadCount;

void gtthread_init( long period );
int gtthread_create( gtthread_t *thread, void *(*start_routine)(void *), void *arg );
int gtthread_join( gtthread_t thread,void **status );
void gtthread_exit( void *retval );
void gtthread_yield( void );
int gtthread_equal( gtthread_t t1, gtthread_t t2 );
int gtthread_cancel( gtthread_t thread );
gtthread_t gtthread_self( void );

#endif








