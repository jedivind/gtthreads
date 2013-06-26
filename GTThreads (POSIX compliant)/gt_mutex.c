#include "gt_mutex.h"

int gtthread_mutex_init( gtthread_mutex_t *mutex ) {
	if( mutex->lock == 1 ) return -1;

	__asm__ __volatile__ (
							"mov $0x00, %%rax\n\t"
							"xchg %%rax, %0\n\t"
							"mov $0xFFFFFFFFFFFFFFFF, %%rax\n\t"
							"xchg %%rax, %1\n\t"
							:"=r" (mutex->lock), "=r" (mutex->owner)
							:
							:"%rax"
						 );

	return 0;
}

int gtthread_mutex_lock( gtthread_mutex_t *mutex ){

	if( ( mutex->owner ) == threadList[currentThread].threadid ){
		return -1;
	}

	while( mutex->lock !=0 && mutex->owner != threadList[currentThread].threadid )
	gtthread_yield();

	__asm__ __volatile__(
						"mov $0x01, %%rax\n\t"
						"xchg %%rax, %0\n\t"
						"mov %2, %%rax\n\t"
						"xchg %%rax, %1\n\t"
						:"=r" (mutex->lock), "=r" (mutex->owner)
						:"r" (threadList[currentThread].threadid)
						:"%rax"
						);

	return 0;
}

int gtthread_mutex_unlock( gtthread_mutex_t *mutex ){

	if( mutex->lock == 1 && mutex->owner == threadList[currentThread].threadid ){

		__asm__ __volatile__ (
								"mov $0x00, %%rax\n\t"
								"xchg %%rax, %0\n\t"
								"mov $0xFFFFFFFFFFFFFFFF, %%rax\n\t"
								"xchg %%rax, %1\n\t"
								:"=r" (mutex->lock), "=r" (mutex->owner)
								 :
								 :"%rax"
							 );

		return 0;
	}
	return -1;
}
