#ifndef __GT_SIGNALS__
#define __GT_SIGNALS__
#include "gtthread.h"
#include "gt_mutex.h"
#include "gt_signals.h"
#include "gt_atomic_ops.h"
#include "gt_context.h"


#include <signal.h>
#include <unistd.h>

extern struct sigaction scheduling_preemption_handler;
extern struct itimerval timequant;

//Signal operations
extern void gtthread_block_signal(int signo);
extern void gtthread_unblock_signal(int signo);
extern void init_itimerval( long period );
extern void init_sigtimer();
extern void init_sched_preempt_handler();
extern void sighandler( int sig_nr, siginfo_t* info, void *old_context );

#endif
