#ifndef __GT_PQ_H
#define __GT_PQ_H

#define MAX_UTHREAD_PRIORITY 32
#define MAX_UTHREAD_GROUPS 32
#define DEFAULT_UTHREAD_PRIORITY 16
#define NICE_0_WEIGHT 1024
#define LONG_MAX 0x7FFFFFFFL

#ifndef _RBTREE_H_
#define INDENT_STEP  4

struct kthread_runqueue_t;

//RB - TREE Structures and functions


/* Copyright (c) 2012 the authors listed at the following URL, and/or
the authors of referenced articles or incorporated external code:
http://en.literateprograms.org/Red-black_tree_(C)?action=history&offset=20090121005050

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Retrieved from: http://en.literateprograms.org/Red-black_tree_(C)?oldid=16016
*/


extern int compare_int(void*, void*);

enum rbtree_node_color { RED, BLACK };

typedef struct rbtree_node_t {
    void* key; //vruntime
    void* value; // The uthread
    	
    struct rbtree_node_t* left;
    struct rbtree_node_t* right;
    struct rbtree_node_t* parent;
    enum rbtree_node_color color;
} *rbtree_node;

typedef struct rbtree_t {
    rbtree_node root;
    int nr_running; //Number of running uthreads in CFS
    int load_weight; // Load of CFS
} *rbtree;

typedef int (*compare_func)(void* left, void* right);
extern uthread_struct_t* find_leftmost_node(rbtree);
extern rbtree_node find_leftmost(rbtree); //Self defined function to find the leftmost node in the tree.


extern void print_tree_helper(rbtree_node, int);
extern void print_tree(rbtree);

extern rbtree rbtree_create();
void* rbtree_lookup(rbtree t, void* key, compare_func compare);
void rbtree_insert(rbtree t, void* key, void* value, compare_func compare);
void rbtree_delete(rbtree t, void* key, compare_func compare);

#endif

//End of RB TREE structures & functions.


TAILQ_HEAD(uthread_head, uthread_struct);
typedef struct uthread_head uthread_head_t;



typedef struct __kthread_runqueue
{
	rbtree cfs_rq;

	int min_vruntime; //vruntime of the leftmost node in the RB tree on this runqueue.	
	gt_spinlock_t kthread_runqlock;

	uthread_struct_t *cur_uthread;	// Current running uthread 
	unsigned int reserved0;
	int inittimer;
	
} kthread_runqueue_t;

//CFS FUNCTIONS
extern void update_curr(struct timeval tv, uthread_struct_t *);
extern void update_min_vruntime(kthread_runqueue_t *kthread_runq, unsigned long vruntime);
extern int sched_period();
extern void check_preempt_tick();
// End of CFS functions.




/* XXX: Move it to gt_sched.h later */


/* NOTE: kthread active/expires/zombie use the same link(uthread_runq) in uthread_struct.
 * This is perfectly fine since active/expires/zombie are mutually exclusive. */

/* NOTE: Each kthread simulates a virtual processor.
 * Since we are running only one uthread on a kthread
 * at any given time, we (typically) do not need a runqlock 
 * when executing uthreads. But, runqlock is necessary to allow 
 * a uthread_create from any kthread to post a uthread to 
 * any kthread's runq.
 * There is not much performance penalty (only one additional check), 
 * since we are using spinlocks. */

/* NOTE: We can have locks at finer granularity than kthread_runqlock.
 * For instance, a lock for each queue. But, for the current design,
 * it will be an overkill. So, we go with one *GIANT* lock for the
 * entire kthread runqueue */


/* only lock protected versions are exported */
//extern void init_runqueue(runqueue_t *runq);
extern void add_to_runqueue(rbtree runq, gt_spinlock_t *runq_lock, uthread_struct_t *u_elem);
extern void rem_from_runqueue(rbtree runq, gt_spinlock_t *runq_lock, uthread_struct_t *u_elem);
//extern void switch_runqueue(runqueue_t *from_runq, gt_spinlock_t *from_runqlock, 
//				runqueue_t *to_runq, gt_spinlock_t *to_runqlock, uthread_struct_t *u_elem);


//extern void print_runq_stats(runqueue_t *, char *);
/* kthread runqueue */
extern void kthread_init_runqueue(kthread_runqueue_t *kthread_runq);

/* Find the highest priority uthread.
 * Called by kthread handling VTALRM. */
extern uthread_struct_t *sched_find_best_uthread(kthread_runqueue_t *kthread_runq);

/* Find the highest priority uthread from uthread_group u_gid.
 * Called by kthread handling SIGUSR1(RELAYED signal).
 * Also globally sets the penalty, if choosing a lower priority uthread. */
extern uthread_struct_t *sched_find_best_uthread_group(kthread_runqueue_t *kthread_runq);


#endif






