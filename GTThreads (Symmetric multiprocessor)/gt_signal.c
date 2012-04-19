#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sched.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <assert.h>

#include "gt_signal.h"


/**********************************************************************/
/* kthread signal handling */

extern void kthread_install_sighandler(int signo, void (*handler)(int))
{
	sigset_t set;
	struct sigaction act;

	/* Setup the handler */
	act.sa_handler = handler;
	act.sa_flags = SA_RESTART;
	sigaction(signo, &act,0);

	/* Unblock the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

	return;
}

extern void kthread_block_signal(int signo)
{
	sigset_t set;

	/* Block the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_BLOCK, &set, NULL);

	return;
}

extern void kthread_unblock_signal(int signo)
{
	sigset_t set;

	/* Unblock the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

	return;
}

extern void kthread_init_vtalrm_timeslice()
{
	struct itimerval timeslice;
	
	timeslice.it_interval.tv_sec = KTHREAD_VTALRM_SEC;
	timeslice.it_interval.tv_usec = sched_timer();// KTHREAD_VTALRM_USEC;
	timeslice.it_value.tv_sec = KTHREAD_VTALRM_SEC;
	timeslice.it_value.tv_usec = sched_timer();//KTHREAD_VTALRM_USEC;
	//printf("VTALRM TIMER VAL IS %d\n", (int)timeslice.it_interval.tv_usec);
	setitimer(ITIMER_VIRTUAL,&timeslice,NULL);
	
	return;
}

extern void kthread_init_alrm_timeslice()
{
	struct itimerval timeslice;

	timeslice.it_interval.tv_sec = KTHREAD_VTALRM_SEC;
	timeslice.it_interval.tv_usec = preempt_timer();// KTHREAD_VTALRM_USEC;
	timeslice.it_value.tv_sec = KTHREAD_VTALRM_SEC;
	timeslice.it_value.tv_usec = preempt_timer();//KTHREAD_VTALRM_USEC;
	
	setitimer(ITIMER_REAL,&timeslice,NULL);
	return;
}

