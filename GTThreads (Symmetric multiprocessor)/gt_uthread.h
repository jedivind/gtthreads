#ifndef __GT_UTHREAD_H
#define __GT_UTHREAD_H

/* User-level thread implementation (using alternate signal stacks) */

typedef unsigned int uthread_t;
typedef unsigned int uthread_group_t;

//static void (preempt_uthread)(int);
/* uthread states */
#define UTHREAD_INIT 0x01
#define UTHREAD_RUNNABLE 0x02
#define UTHREAD_RUNNING 0x04
#define UTHREAD_CANCELLED 0x08
#define UTHREAD_DONE 0x10
#define UTHREAD_PREEMPT 0x12
struct __kthread_runqueue;
/* uthread struct : has all the uthread context info */
typedef struct uthread_struct
{
	
	int uthread_state; /* UTHREAD_INIT, UTHREAD_RUNNABLE, UTHREAD_RUNNING, UTHREAD_CANCELLED, UTHREAD_DONE */
	int uthread_priority; /* uthread running priority */
	int cpu_id; /* cpu it is currently executing on */
	int last_cpu_id; /* last cpu it was executing on */
	
	unsigned long int vruntime; //Vruntime of the thread.
	long load_weight; //Weight of the thread. Typically NICE_0_WEIGHT

	uthread_t uthread_tid; /* thread id */
	uthread_group_t uthread_gid; /* thread group id  */
	int (*uthread_func)(void*);
	void *uthread_arg;
	
	int vgroup; //Virtual group of the thread. Used to collect vruntime stats.

	void *exit_status; /* exit status */
	int reserved1;
	int reserved2;
	int reserved3;
	
	int yielded; //Set when the thread yields CPU.
	int on_rq; // Set if thread is on a CFS tree.

	struct __kthread_runqueue *runq;
	sigjmp_buf uthread_env; /* 156 bytes : save user-level thread context*/
	stack_t uthread_stack; /* 12 bytes : user-level thread stack */

} uthread_struct_t;




extern void uthread_schedule(uthread_struct_t * (*kthread_best_sched_uthread)(struct __kthread_runqueue *));
#endif
