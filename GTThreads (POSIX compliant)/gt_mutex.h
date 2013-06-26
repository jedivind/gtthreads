#ifndef __GT_MUTEX__
#define __GT_MUTEX__
#include "gtthread.h"
#include "gt_mutex.h"
#include "gt_signals.h"
#include "gt_atomic_ops.h"
#include "gt_context.h"

typedef struct gtthread_mutex_t{
	long lock;
	gtthread_t owner;
}gtthread_mutex_t;

//Mutex operations
extern int gtthread_mutex_init( gtthread_mutex_t *mutex );
extern int  gtthread_mutex_lock( gtthread_mutex_t *mutex );
extern int  gtthread_mutex_unlock( gtthread_mutex_t *mutex );

#endif
