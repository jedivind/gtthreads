#ifndef __GT_SIGNAL_H
#define __GT_SIGNAL_H

/**********************************************************************/
/* kthread signal handling */
extern void kthread_install_sighandler(int signo, void (*handler)(int));
extern void kthread_block_signal(int signo);
extern void kthread_unblock_signal(int signo);

#define KTHREAD_VTALRM_SEC 0
#define KTHREAD_VTALRM_USEC 100000
extern void kthread_init_vtalrm_timeslice();
extern void kthread_init_alrm_timeslice();
#endif
