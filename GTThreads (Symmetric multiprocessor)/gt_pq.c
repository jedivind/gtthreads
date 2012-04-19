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

#include "gt_include.h"

unsigned int sched_latency = 20000000ULL; 
unsigned int sched_nr_latency = 5;
unsigned int sched_min_granularity = 4000000ULL;



//RB - TREE FUNCTIONS. Taken from Open Source RB-TREE library http://en.literateprograms.org/Red-black_tree_%28C%29
extern int compare_int(void* leftp, void* rightp) {
    int left = (int)leftp;
    int right = (int)rightp;
    if (left < right) 
        return -1;
    else if (left > right)
        return 1;
    else {
        assert (left == right);
        return 0;
    }
}


extern void print_tree(rbtree t) {
    print_tree_helper(t->root, 0);
    puts("");
}

extern void print_tree_helper(rbtree_node n, int indent) {
    int i;
    if (n == NULL) {
        fputs("<empty tree>", stdout);
        return;
    }
    if (n->right != NULL) {
        print_tree_helper(n->right, indent + INDENT_STEP);
    }
    for(i=0; i<indent; i++)
        fputs(" ", stdout);
    if (n->color == BLACK)
        printf("%d\n", (int)n->key);
    else
        printf("<%d>\n", (int)n->key);
    if (n->left != NULL) {
        print_tree_helper(n->left, indent + INDENT_STEP);
    }
}
//RB - TREE FUNCTIONS END

/**********************************************************************/
/* runqueue operations */
static void __add_to_cfs_runqueue(rbtree runq, uthread_struct_t *u_elm);
static void __rem_from_cfs_runqueue(rbtree runq, uthread_struct_t *u_elm);

//static void __add_to_runqueue(runqueue_t *runq, uthread_struct_t *u_elm);
//static void __rem_from_runqueue(runqueue_t *runq, uthread_struct_t *u_elm);
/**********************************************************************/
/* runqueue operations */
static inline void __add_to_cfs_runqueue(rbtree runq, uthread_struct_t *u_elem)
{
	rbtree_insert(runq, (void*) u_elem->vruntime, (void*)u_elem, compare_int);
	runq->nr_running++;
	return;
}

static inline void __rem_from_cfs_runqueue(rbtree runq, uthread_struct_t *u_elem)
{
	rbtree_delete(runq, (void*) u_elem->vruntime, compare_int);	
	runq->nr_running--;
	return;
}



/**********************************************************************/
/* Exported runqueue operations */


extern void add_to_cfs_runqueue(rbtree runq, gt_spinlock_t *runq_lock, uthread_struct_t *u_elem)
{
	gt_spin_lock(runq_lock);
	runq_lock->holder = 0x02;
	__add_to_cfs_runqueue(runq, u_elem);
	gt_spin_unlock(runq_lock);
	return;
}

extern void rem_from_cfs_runqueue(rbtree runq, gt_spinlock_t *runq_lock, uthread_struct_t *u_elem)
{
	gt_spin_lock(runq_lock);
	runq_lock->holder = 0x03;
	__rem_from_cfs_runqueue(runq, u_elem);
	gt_spin_unlock(runq_lock);
	return;
}

/*extern void switch_runqueue(runqueue_t *from_runq, gt_spinlock_t *from_runqlock, 
				runqueue_t *to_runq, gt_spinlock_t *to_runqlock, uthread_struct_t *u_elem)
{
	rem_from_runqueue(from_runq, from_runqlock, u_elem);
	add_to_runqueue(to_runq, to_runqlock, u_elem);
	return;
}*/


/**********************************************************************/

extern void kthread_init_runqueue(kthread_runqueue_t *kthread_runq)
{
	
	gt_spinlock_init(&(kthread_runq->kthread_runqlock));
	kthread_runq->cfs_rq = rbtree_create();
	
	return;
}

#if 0
void print_runq_stats(runqueue_t *runq, char *runq_str)
{
	int inx;
	printf("******************************************************\n");
	printf("Run queue(%s) state : \n", runq_str);
	printf("******************************************************\n");
	printf("uthreads details - (tot:%d , mask:%x)\n", runq->uthread_tot, runq->uthread_mask);
	printf("******************************************************\n");
	printf("uthread priority details : \n");
	for(inx=0; inx<MAX_UTHREAD_PRIORITY; inx++)
		printf("uthread priority (%d) - (tot:%d)\n", inx, runq->uthread_prio_tot[inx]);
	return;
	printf("******************************************************\n");
	printf("uthread group details : \n");
	for(inx=0; inx<MAX_UTHREAD_GROUPS; inx++)
		printf("uthread group (%d) - (tot:%d , mask:%x)\n", inx, runq->uthread_group_tot[inx], runq->uthread_group_mask[inx]);
	printf("******************************************************\n");
	return;
}
#endif

extern uthread_struct_t *sched_find_best_uthread(kthread_runqueue_t *kthread_runq)
{
	/* [1] Tries to find the highest priority RUNNABLE uthread in active-runq.
	 * [2] Found - Jump to [FOUND]
	 * [3] Switches runqueues (active/expires)
	 * [4] Repeat [1] through [2]
	 * [NOT FOUND] Return NULL(no more jobs)
	 * [FOUND] Remove uthread from pq and return it. */

	rbtree rbrunq = kthread_runq->cfs_rq;
	rbtree_node node = find_leftmost(rbrunq);
	
	uthread_struct_t *u_obj;
	gt_spin_lock(&(kthread_runq->kthread_runqlock));
	kthread_runq->kthread_runqlock.holder = 0x04;



	if(rbrunq->nr_running == 0 || !node){ //If there are no more nodes to schedule, return NULL.
	gt_spin_unlock(&(kthread_runq->kthread_runqlock));
	return NULL;
	}

	u_obj = (uthread_struct_t *) node->value; //Else get the uthread in the leftmost node.
	update_min_vruntime(kthread_runq, u_obj->vruntime); //Set the min_vruntime of the runqueue to this uthread's vruntime.
	__rem_from_cfs_runqueue(rbrunq,u_obj); //Remove the object from the CFS tree.
	gt_spin_unlock(&(kthread_runq->kthread_runqlock));
	return u_obj; //Return the leftmost node.
#if 0
	printf("cpu(%d) : sched best uthread(id:%d, group:%d)\n", u_obj->cpu_id, u_obj->uthread_tid, u_obj->uthread_gid);
#endif
}



/* XXX: More work to be done !!! */
extern gt_spinlock_t uthread_group_penalty_lock;
extern unsigned int uthread_group_penalty;
/*
extern uthread_struct_t *sched_find_best_uthread_group(kthread_runqueue_t *kthread_runq)
{
	/* [1] Tries to find a RUNNABLE uthread in active-runq from u_gid.
	 * [2] Found - Jump to [FOUND]
	 * [3] Tries to find a thread from a group with least threads in runq (XXX: NOT DONE)
	 * - [Tries to find the highest priority RUNNABLE thread (XXX: DONE)]
	 * [4] Found - Jump to [FOUND]
	 * [5] Switches runqueues (active/expires)
	 * [6] Repeat [1] through [4]
	 * [NOT FOUND] Return NULL(no more jobs)
	 * [FOUND] Remove uthread from pq and return it. */
	
/*	rbtree rbrunq = kthread_runq->cfs_rq;
	
	
	runqueue_t *runq;
	prio_struct_t *prioq;
	uthread_head_t *u_head;
	uthread_struct_t *u_obj;
	unsigned int uprio, ugroup, mask;
	uthread_group_t u_gid;

#ifndef COSCHED
	return sched_find_best_uthread(kthread_runq);
#endif
	
	/* XXX: Read u_gid from global uthread-select-criterion */
/*	u_gid = 0;
	runq = kthread_runq->active_runq;

	if(!runq->uthread_mask)
	{ /* No jobs in active. switch runqueue */
/*		assert(!runq->uthread_tot);
		//kthread_runq->active_runq = kthread_runq->expires_runq;
		//kthread_runq->expires_runq = runq;

		//runq = kthread_runq->expires_runq;
		if(!runq->uthread_mask)
		{
			assert(!runq->uthread_tot);
			return NULL;
		}
	}

	
	if(!(mask = runq->uthread_group_mask[u_gid]))
	{ /* No uthreads in the desired group */
/*		assert(!runq->uthread_group_tot[u_gid]);
		return (sched_find_best_uthread(kthread_runq));
	}

	/* Find the highest priority bucket for u_gid */
	//uprio = LOWEST_BIT_SET(mask);

	/* Take out a uthread from the bucket. Return it. */
	//u_head = &(runq->prio_array[uprio].group[u_gid]);
	//u_obj = TAILQ_FIRST(u_head);
	//rem_from_runqueue(runq, &(kthread_runq->kthread_runqlock), u_obj);
/*	uthread_struct_t * cur_uthread = find_leftmost_node(rbrunq);
	if(cur_uthread->uthread_state == UTHREAD_DONE)return 0;
	
	__rem_from_runqueue(runq, NULL);
	rbtree_delete(rbrunq, &cur_uthread->vruntime, compare_int);
	return(cur_uthread);
	
}*/

#if 0
/*****************************************************************************************/
/* Main Test Function */

runqueue_t active_runqueue, expires_runqueue;

#define MAX_UTHREADS 1000
uthread_struct_t u_objs[MAX_UTHREADS];

static void fill_runq(runqueue_t *runq)
{
	uthread_struct_t *u_obj;
	int inx;
	/* create and insert */
	for(inx=0; inx<MAX_UTHREADS; inx++)
	{
		u_obj = &u_objs[inx];
		u_obj->uthread_tid = inx;
		u_obj->uthread_gid = (inx % MAX_UTHREAD_GROUPS);
		u_obj->uthread_priority = (inx % MAX_UTHREAD_PRIORITY);
		__add_to_runqueue(runq, u_obj);
		printf("Uthread (id:%d , prio:%d) inserted\n", u_obj->uthread_tid, u_obj->uthread_priority);
	}

	return;
}

static void print_runq_stats(runqueue_t *runq, char *runq_str)
{
	int inx;
	printf("******************************************************\n");
	printf("Run queue(%s) state : \n", runq_str);
	printf("******************************************************\n");
	printf("uthreads details - (tot:%d , mask:%x)\n", runq->uthread_tot, runq->uthread_mask);
	printf("******************************************************\n");
	printf("uthread priority details : \n");
	for(inx=0; inx<MAX_UTHREAD_PRIORITY; inx++)
		printf("uthread priority (%d) - (tot:%d)\n", inx, runq->uthread_prio_tot[inx]);
	printf("******************************************************\n");
	printf("uthread group details : \n");
	for(inx=0; inx<MAX_UTHREAD_GROUPS; inx++)
		printf("uthread group (%d) - (tot:%d , mask:%x)\n", inx, runq->uthread_group_tot[inx], runq->uthread_group_mask[inx]);
	printf("******************************************************\n");
	return;
}

static void change_runq(runqueue_t *from_runq, runqueue_t *to_runq)
{
	uthread_struct_t *u_obj;
	int inx;
	/* Remove and delete */
	for(inx=0; inx<MAX_UTHREADS; inx++)
	{
		u_obj = &u_objs[inx];
		switch_runqueue(from_runq, to_runq, u_obj);
		printf("Uthread (id:%d , prio:%d) moved\n", u_obj->uthread_tid, u_obj->uthread_priority);
	}

	return;
}


static void empty_runq(runqueue_t *runq)
{
	uthread_struct_t *u_obj;
	int inx;
	/* Remove and delete */
	for(inx=0; inx<MAX_UTHREADS; inx++)
	{
		u_obj = &u_objs[inx];
		__rem_from_runqueue(runq, u_obj);
		printf("Uthread (id:%d , prio:%d) removed\n", u_obj->uthread_tid, u_obj->uthread_priority);
	}

	return;
}

int main()
{
	runqueue_t *active_runq, *expires_runq;
	uthread_struct_t *u_obj;
	int inx;

	active_runq = &active_runqueue;
	expires_runq = &expires_runqueue;

	init_runqueue(active_runq);
	init_runqueue(expires_runq);

	fill_runq(active_runq);
	print_runq_stats(active_runq, "ACTIVE");
	print_runq_stats(expires_runq, "EXPIRES");
	change_runq(active_runq, expires_runq);
	print_runq_stats(active_runq, "ACTIVE");
	print_runq_stats(expires_runq, "EXPIRES");
	empty_runq(expires_runq);
	print_runq_stats(active_runq, "ACTIVE");
	print_runq_stats(expires_runq, "EXPIRES");
	
	return 0;
}

#endif



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


typedef rbtree_node node;
typedef enum rbtree_node_color color;


static node grandparent(node n);
static node sibling(node n);
static node uncle(node n);
static void verify_properties(rbtree t);
static void verify_property_1(node root);
static void verify_property_2(node root);
static color node_color(node n);
static void verify_property_4(node root);
static void verify_property_5(node root);
static void verify_property_5_helper(node n, int black_count, int* black_count_path);

static node new_node(void* key, void* value, color node_color, node left, node right);
static node lookup_node(rbtree t, void* key, compare_func compare);
static void rotate_left(rbtree t, node n);
static void rotate_right(rbtree t, node n);

static void replace_node(rbtree t, node oldn, node newn);
static void insert_case1(rbtree t, node n);
static void insert_case2(rbtree t, node n);
static void insert_case3(rbtree t, node n);
static void insert_case4(rbtree t, node n);
static void insert_case5(rbtree t, node n);
static node maximum_node(node root);
static void delete_case1(rbtree t, node n);
static void delete_case2(rbtree t, node n);
static void delete_case3(rbtree t, node n);
static void delete_case4(rbtree t, node n);
static void delete_case5(rbtree t, node n);
static void delete_case6(rbtree t, node n);

node grandparent(node n) {
    assert (n != NULL);
    assert (n->parent != NULL); /* Not the root node */
    assert (n->parent->parent != NULL); /* Not child of root */
    return n->parent->parent;
}

node sibling(node n) {
    assert (n != NULL);
    assert (n->parent != NULL); /* Root node has no sibling */
    if (n == n->parent->left)
        return n->parent->right;
    else
        return n->parent->left;
}

node uncle(node n) {
    assert (n != NULL);
    assert (n->parent != NULL); /* Root node has no uncle */
    assert (n->parent->parent != NULL); /* Children of root have no uncle */
    return sibling(n->parent);
}

void verify_properties(rbtree t) {
#ifdef VERIFY_RBTREE
    verify_property_1(t->root);
    verify_property_2(t->root);
    /* Property 3 is implicit */
    verify_property_4(t->root);
    verify_property_5(t->root);
#endif
}

void verify_property_1(node n) {
    assert(node_color(n) == RED || node_color(n) == BLACK);
    if (n == NULL) return;
    verify_property_1(n->left);
    verify_property_1(n->right);
}

void verify_property_2(node root) {
    assert(node_color(root) == BLACK);
}

color node_color(node n) {
    return n == NULL ? BLACK : n->color;
}

void verify_property_4(node n) {
    if (node_color(n) == RED) {
        assert (node_color(n->left)   == BLACK);
        assert (node_color(n->right)  == BLACK);
        assert (node_color(n->parent) == BLACK);
    }
    if (n == NULL) return;
    verify_property_4(n->left);
    verify_property_4(n->right);
}

void verify_property_5(node root) {
    int black_count_path = -1;
    verify_property_5_helper(root, 0, &black_count_path);
}

void verify_property_5_helper(node n, int black_count, int* path_black_count) {
    if (node_color(n) == BLACK) {
        black_count++;
    }
    if (n == NULL) {
        if (*path_black_count == -1) {
            *path_black_count = black_count;
        } else {
            assert (black_count == *path_black_count);
        }
        return;
    }
    verify_property_5_helper(n->left,  black_count, path_black_count);
    verify_property_5_helper(n->right, black_count, path_black_count);
}

rbtree rbtree_create() {
    rbtree t = malloc(sizeof(struct rbtree_t));
    t->root = NULL;
    verify_properties(t);
    return t;
}

node new_node(void* key, void* value, color node_color, node left, node right) {
    node result = malloc(sizeof(struct rbtree_node_t));
    result->key = key;
    result->value = value;
    result->color = node_color;
    result->left = left;
    result->right = right;
    if (left  != NULL)  left->parent = result;
    if (right != NULL) right->parent = result;
    result->parent = NULL;
    return result;
}

node lookup_node(rbtree t, void* key, compare_func compare) {
    node n = t->root;
    while (n != NULL) {
        int comp_result = compare(key, n->key);
        if (comp_result == 0) {
            return n;
        } else if (comp_result < 0) {
            n = n->left;
        } else {
            assert(comp_result > 0);
            n = n->right;
        }
    }
    return n;
}

void* rbtree_lookup(rbtree t, void* key, compare_func compare) {
    node n = lookup_node(t, key, compare);
    return n == NULL ? NULL : n->value;
}

void rotate_left(rbtree t, node n) {
    node r = n->right;
    replace_node(t, n, r);
    n->right = r->left;
    if (r->left != NULL) {
        r->left->parent = n;
    }
    r->left = n;
    n->parent = r;
}

void rotate_right(rbtree t, node n) {
    node L = n->left;
    replace_node(t, n, L);
    n->left = L->right;
    if (L->right != NULL) {
        L->right->parent = n;
    }
    L->right = n;
    n->parent = L;
}

void replace_node(rbtree t, node oldn, node newn) {
    if (oldn->parent == NULL) {
        t->root = newn;
    } else {
        if (oldn == oldn->parent->left)
            oldn->parent->left = newn;
        else
            oldn->parent->right = newn;
    }
    if (newn != NULL) {
        newn->parent = oldn->parent;
    }
}

void rbtree_insert(rbtree t, void* key, void* value, compare_func compare) {
    node inserted_node = new_node(key, value, RED, NULL, NULL);
    if (t->root == NULL) {
        t->root = inserted_node;
    } else {
        node n = t->root;
        while (1) {
            int comp_result = compare(key, n->key);
            if (comp_result == 0) {
                n->value = value;
                /* inserted_node isn't going to be used, don't leak it */
                free (inserted_node);
                return;
            } else if (comp_result < 0) {
                if (n->left == NULL) {
                    n->left = inserted_node;
                    break;
                } else {
                    n = n->left;
                }
            } else {
                assert (comp_result > 0);
                if (n->right == NULL) {
                    n->right = inserted_node;
                    break;
                } else {
                    n = n->right;
                }
            }
        }
        inserted_node->parent = n;
    }
    insert_case1(t, inserted_node);
    verify_properties(t);
}

void insert_case1(rbtree t, node n) {
    if (n->parent == NULL)
        n->color = BLACK;
    else
        insert_case2(t, n);
}

void insert_case2(rbtree t, node n) {
    if (node_color(n->parent) == BLACK)
        return; /* Tree is still valid */
    else
        insert_case3(t, n);
}

void insert_case3(rbtree t, node n) {
    if (node_color(uncle(n)) == RED) {
        n->parent->color = BLACK;
        uncle(n)->color = BLACK;
        grandparent(n)->color = RED;
        insert_case1(t, grandparent(n));
    } else {
        insert_case4(t, n);
    }
}

void insert_case4(rbtree t, node n) {
    if (n == n->parent->right && n->parent == grandparent(n)->left) {
        rotate_left(t, n->parent);
        n = n->left;
    } else if (n == n->parent->left && n->parent == grandparent(n)->right) {
        rotate_right(t, n->parent);
        n = n->right;
    }
    insert_case5(t, n);
}

void insert_case5(rbtree t, node n) {
    n->parent->color = BLACK;
    grandparent(n)->color = RED;
    if (n == n->parent->left && n->parent == grandparent(n)->left) {
        rotate_right(t, grandparent(n));
    } else {
        assert (n == n->parent->right && n->parent == grandparent(n)->right);
        rotate_left(t, grandparent(n));
    }
}

void rbtree_delete(rbtree t, void* key, compare_func compare) {
    node child;
    node n = lookup_node(t, key, compare);
    if (n == NULL) return;  /* Key not found, do nothing */
    if (n->left != NULL && n->right != NULL) {
        /* Copy key/value from predecessor and then delete it instead */
        node pred = maximum_node(n->left);
        n->key   = pred->key;
        n->value = pred->value;
        n = pred;
    }

    assert(n->left == NULL || n->right == NULL);
    child = n->right == NULL ? n->left  : n->right;
    if (node_color(n) == BLACK) {
        n->color = node_color(child);
        delete_case1(t, n);
    }
    replace_node(t, n, child);
    if (n->parent == NULL && child != NULL)
        child->color = BLACK;
    free(n);

    verify_properties(t);
}

static node maximum_node(node n) {
    assert (n != NULL);
    while (n->right != NULL) {
        n = n->right;
    }
    return n;
}

void delete_case1(rbtree t, node n) {
    if (n->parent == NULL)
        return;
    else
        delete_case2(t, n);
}

void delete_case2(rbtree t, node n) {
    if (node_color(sibling(n)) == RED) {
        n->parent->color = RED;
        sibling(n)->color = BLACK;
        if (n == n->parent->left)
            rotate_left(t, n->parent);
        else
            rotate_right(t, n->parent);
    }
    delete_case3(t, n);
}

void delete_case3(rbtree t, node n) {
    if (node_color(n->parent) == BLACK &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->left) == BLACK &&
        node_color(sibling(n)->right) == BLACK)
    {
        sibling(n)->color = RED;
        delete_case1(t, n->parent);
    }
    else
        delete_case4(t, n);
}

void delete_case4(rbtree t, node n) {
    if (node_color(n->parent) == RED &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->left) == BLACK &&
        node_color(sibling(n)->right) == BLACK)
    {
        sibling(n)->color = RED;
        n->parent->color = BLACK;
    }
    else
        delete_case5(t, n);
}

void delete_case5(rbtree t, node n) {
    if (n == n->parent->left &&
        node_color(sibling(n)) == BLACK &&
        node_color(sibling(n)->left) == RED &&
        node_color(sibling(n)->right) == BLACK)
    {
        sibling(n)->color = RED;
        sibling(n)->left->color = BLACK;
        rotate_right(t, sibling(n));
    }
    else if (n == n->parent->right &&
             node_color(sibling(n)) == BLACK &&
             node_color(sibling(n)->right) == RED &&
             node_color(sibling(n)->left) == BLACK)
    {
        sibling(n)->color = RED;
        sibling(n)->right->color = BLACK;
        rotate_left(t, sibling(n));
    }
    delete_case6(t, n);
}

void delete_case6(rbtree t, node n) {
    sibling(n)->color = node_color(n->parent);
    n->parent->color = BLACK;
    if (n == n->parent->left) {
        assert (node_color(sibling(n)->right) == RED);
        sibling(n)->right->color = BLACK;
        rotate_left(t, n->parent);
    }
    else
    {
        assert (node_color(sibling(n)->left) == RED);
        sibling(n)->left->color = BLACK;
        rotate_right(t, n->parent);
    }
}

//////////////////END OF RBTREE FUNCTIONS///

//SELF DEFINED FUNCTIONS, both CFS and RB tree.
extern rbtree_node find_leftmost(rbtree t)
{	
	rbtree_node node = t->root;
	if(!node) return NULL;
	if(node->left != NULL)
	while(node->left != NULL) node = node->left;
	
	return node;
}

extern void update_min_vruntime(kthread_runqueue_t *kthread_runq, unsigned long vruntime)
{
	kthread_runq->min_vruntime = vruntime;	
}

static inline int min_vruntime(int min_vruntime, int vruntime)
{
	int delta = vruntime - min_vruntime;
	if(delta < 0) min_vruntime = vruntime;
	return min_vruntime;
}

int sched_period(int nr_running)
{
         int period = sched_latency;
         int nr_latency = sched_nr_latency;
 
         if (nr_running > nr_latency) {
                 period = sched_min_granularity;
                 period *= nr_running;
         }
 
         return period;
}

extern unsigned int sched_timer()
{

return sched_latency;
}

extern unsigned int preempt_timer()
{
return sched_min_granularity;
}

void set_sched_timer(int nr_running)
{
	sched_latency = sched_period(nr_running);
}

void update_curr(struct timeval tv1, uthread_struct_t *cur_uthread)
{
	struct timeval tv2;
	gettimeofday(&tv2, NULL);
	unsigned int vtime = (tv2.tv_sec*100000+tv2.tv_usec)-(tv1.tv_sec*100000+tv1.tv_usec);
	cur_uthread->vruntime += vtime;
	if(&cur_uthread->vgroup != NULL){
	gt_spin_lock(stat_lock);
	vruntime[cur_uthread->uthread_gid]+= (double)vtime;
	vruntime_thread[cur_uthread->uthread_gid][cur_uthread->uthread_tid]+= (double)vtime;
	gt_spin_unlock(stat_lock);	
	}
	
	
}

