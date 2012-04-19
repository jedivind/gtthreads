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
#include <math.h>
#include <stdlib.h>

#include "gt_include.h"



#define ROWS 32
#define COLS ROWS
#define SIZE COLS

#define SIZE_FIRST SIZE    //32//   //First set of 32 threads out of 128 threads multiply matrices of size 32
#define SIZE_SECOND SIZE*2 //64//   //Second set of 32 threads out of 128 threads multiply matrices of size 64
#define SIZE_THIRD SIZE*4  //128//   //Third set of 32 threads out of 128 threads multiply matrices of size 128
#define SIZE_FOURTH SIZE*8 //256//   //Last set of 32 threads out of 128 threads multiply matrices of size 256 :)


#define NUM_GROUPS 4
#define PER_GROUP_COLS (SIZE/NUM_GROUPS)

#define NUM_THREADS 128
#define PER_THREAD_ROWS (SIZE/NUM_THREADS)


/* A[SIZE][SIZE] X B[SIZE][SIZE] = C[SIZE][SIZE]
 * Let T(g, t) be thread 't' in group 'g'. 
 * T(g, t) is responsible for multiplication : 
 * A(rows)[(t-1)*SIZE -> (t*SIZE - 1)] X B(cols)[(g-1)*SIZE -> (g*SIZE - 1)] */



typedef struct matrix
{
	int m[256][256];

	int rows;
	int cols;
	unsigned int reserved[2];
} matrix_t;


typedef struct __uthread_arg
{
	matrix_t *_A, *_B, *_C;
	unsigned int reserved0;
	unsigned int tid;
	unsigned int gid;
	int start_row; /* start_row -> (start_row + PER_THREAD_ROWS) */
	int start_col; /* start_col -> (start_col + PER_GROUP_COLS) */
	int end_row;
	int end_col;
	

}uthread_arg_t;
	
struct timeval tv1;

static void generate_matrix(matrix_t *mat, int val)
{
	 
	int i,j;
	mat->rows = 256; //Matrices of all threads initialized to size 256, but each thread only multiplies the size it has to. 
	mat->cols = 256;
	for(i = 0; i < mat->rows;i++)
		for( j = 0; j < mat->cols; j++ )
		{
			mat->m[i][j] = val;
		}
	return;
}

static void print_matrix(void *p)
{
	int i, j;
	#define ptr1 ((uthread_arg_t *)p)
	for(i=0;i<ptr1->end_row;i++)
	{
		for(j=0;j<ptr1->end_col;j++)
			printf(" %d ",ptr1->_C->m[i][j]);
		printf("\n");
	}

	return;
}

static void init_matrices(matrix_t *A, matrix_t  *B, matrix_t *C)
{
	generate_matrix(A, 1);
	generate_matrix(B, 1);
	generate_matrix(C, 0);

	return;
}


static void * uthread_mulmat(void *p)
{
	int i, j, k;
	int start_row, end_row;
	int start_col, end_col;
	unsigned int cpuid;
	struct timeval tv2;
#define ptr ((uthread_arg_t *)p)
	
	ptr->_A = (matrix_t*)malloc(sizeof(matrix_t));
	ptr->_B = (matrix_t*)malloc(sizeof(matrix_t));
	ptr->_C = (matrix_t*)malloc(sizeof(matrix_t)); 
	init_matrices(ptr->_A, ptr->_B, ptr->_C);
 	
	i=0; j= 0; k=0;

	start_row = ptr->start_row;
	end_row = ptr->end_row;//(ptr->start_row + PER_THREAD_ROWS);

#ifdef GT_GROUP_SPLIT
	start_col = ptr->start_col;
	end_col = ptr->end_col;//(ptr->start_col + PER_THREAD_ROWS);
#else
	start_col = 0;
	end_col = ptr->end_col;
#endif
	
	
#ifdef GT_THREADS
	cpuid = kthread_cpu_map[kthread_apic_id()]->cpuid;
	fprintf(stderr, "\nThread(id:%d, group:%d, cpu:%d) started",ptr->tid, ptr->gid, cpuid);
#else
	fprintf(stderr, "\nThread(id:%d, group:%d) started",ptr->tid, ptr->gid);
#endif

	for(i = start_row; i < end_row; i++)
		for(j = start_col; j < end_col; j++)
			for(k = 0; k < end_row; k++){ //Only multiply matrix of size the thread actually has to.
				ptr->_C->m[i][j] += ptr->_A->m[i][k] * ptr->_B->m[k][j];
				}

#ifdef GT_THREADS
		fprintf(stderr, "\nThread(id:%d, group:%d) finished (TIME : %lu s and %lu us)",
			ptr->tid, ptr->gid, (tv2.tv_sec - tv1.tv_sec), (tv2.tv_usec - tv1.tv_usec));
#else
	gettimeofday(&tv2,NULL);
	fprintf(stderr, "\nThread(id:%d, group:%d) finished (TIME : %lu s and %lu us)",
			ptr->tid, ptr->gid, (tv2.tv_sec - tv1.tv_sec), (tv2.tv_usec - tv1.tv_usec));
	
	gt_spin_lock(stat_lock);
	exec[ptr->gid] += (double)(((tv2.tv_sec - tv1.tv_sec)) + ((tv2.tv_usec - tv1.tv_usec)/100000));
	exec_thread[ptr->gid][ptr->tid]+= (double)(((tv2.tv_sec - tv1.tv_sec)) + ((tv2.tv_usec - tv1.tv_usec)/100000));
	gt_spin_unlock(stat_lock);
	gt_yield();
#endif
//print_matrix(ptr);
#undef ptr
	
	return 0;
}






uthread_arg_t uargs[NUM_THREADS];
uthread_t utids[NUM_THREADS];

int main()
{
	gt_spinlock_init(stat_lock);
	uthread_arg_t *uarg = (uthread_arg_t*)malloc(sizeof(uthread_arg_t));
	int inx;
	int i=0, j, k, l;
	double sd=0, sd2=0;

	for(j=0; j<4; j++){
	vruntime[j]=0;
	exec[j]=0;
	for(k=0; k<128; k++){
	vruntime_thread[j][k]=0;
	exec_thread[j][k]=0;
	  }
	}

	
	gtthread_app_init();
	
	
	gettimeofday(&tv1,NULL);

	for(inx=0; inx<NUM_THREADS; inx++)
	{
		uarg = &uargs[inx];
		uarg->tid = inx;

		uarg->gid = (inx % NUM_GROUPS);

		uarg->start_row = 0;
		
#ifdef GT_GROUP_SPLIT
		
		uarg->start_col = 0;
#endif
		//Assign the size of matrix each thread has to multiply. 
		//Assign consecutive threads to multiply matrices of different sizes. I thought of assigning the first 32 threads to multiply matrix of SIZE, 
		//the next 32 threads to multiply matrix of SIZE*2 and so on, but this approach sort of more accurately loads the threads uniformly. 
		switch(uarg->gid){
		case 0: { uarg->end_row = SIZE_FIRST; uarg->end_col = SIZE_FIRST; break;}		
		case 1: { uarg->end_row = SIZE_SECOND; uarg->end_col = SIZE_SECOND; break;}
		case 2: { uarg->end_row = SIZE_THIRD; uarg->end_col = SIZE_THIRD; break;}
		case 3: { uarg->end_row = SIZE_FOURTH; uarg->end_col = SIZE_FOURTH; break;}
		}		

		uthread_create(&utids[inx], uthread_mulmat, uarg, uarg->gid);
	}
	
	gtthread_app_exit();

	//STATS/////////////////////////////////////////////////////////////////////

	for(j=0; j<4; j++){
	printf("Mean vruntime for threads in vgroup %d is %lf us\n", j, vruntime[j]/32);
	printf("Mean total execution time for threads in vgroup %d is %lf s\n", j, exec[j]/32);
	}

	for(j=0; j<4; j++){
	  vruntime[j] = vruntime[j]/100000; 
          for(k=0; k<128; k++)
	   {
		if(vruntime_thread[j][k]!=0){
		vruntime_thread[j][k] = (vruntime_thread[j][k])/100000;
		sd += pow((vruntime_thread[j][k]-(vruntime[j]/32)),2);
		//printf("vruntime %f vruntimegp %f sd %f\n", vruntime_thread[j][k], vruntime[j]/32, sd);
		}
		
		if(exec_thread[j][k]!=0){
		sd2 += pow((exec_thread[j][k]-(exec[j]/32)),2);
		}
	   }
	sd = sqrt(sd)/sqrt(32);
	sd2 = sqrt(sd2)/sqrt(32);
	printf("Standard deviation of vruntime for threads in vgroup %d is %lf\n",j,sd);
	printf("Standard deviation of total execution time for threads in vgroup %d is %lf\n",j,sd2);	
	sd=0;
	sd2=0;
	} 

	// fprintf(stderr, "********************************");
	return(0);
}
