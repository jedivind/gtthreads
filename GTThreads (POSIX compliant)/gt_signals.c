#include "gt_signals.h"

struct sigaction scheduling_preemption_handler;
struct itimerval timequant;

void sighandler( int sig_nr, siginfo_t* info, void *old_context ) {

	if ( sig_nr == SIGALRM )
		swapcontext( &threadList[currentThread].context, &scheduler_context );

	else return;
}

void gtthread_block_signal( int signo ){
	sigset_t set;

	/* Block the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_BLOCK, &set, NULL);

return;
}

void gtthread_unblock_signal( int signo ){
	sigset_t set;

	/* Block the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_BLOCK, &set, NULL);

return;
}

void init_itimerval( long period ){

	timequant.it_value.tv_sec = 0;
	timequant.it_value.tv_usec = (long) period;
	timequant.it_interval = timequant.it_value;

}

void init_sigtimer(){

	if( setitimer( ITIMER_REAL, &timequant, NULL ) == 0) {
#ifdef DEBUG
		printf("\n\tTimer initialized\n");
#endif
	}
	else {
		printf("\n\tAn error occurred while initializing timer. Please check the value of period to gtthread_init()\n");
		exit(-1);
	}
	return;
}

void init_sched_preempt_handler(){

	scheduling_preemption_handler.sa_sigaction = sighandler;
	scheduling_preemption_handler.sa_flags = SA_RESTART | SA_SIGINFO;
	sigemptyset( &scheduling_preemption_handler.sa_mask );

	if ( sigaction( SIGALRM, &scheduling_preemption_handler, NULL ) == -1 ) {
		printf("\nAn error occurred while initializing the signal handler for swapping to the scheduler context\n");
		exit(-1);
	}
	return;
}
