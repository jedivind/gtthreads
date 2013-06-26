#include "gt_atomic_ops.h"

 void atomic_increment( int *operand ){

	__asm__ __volatile__(
							"mov %1, %%eax\n\t"
							"add $0x01, %%eax\n\t"
							"xchg %%eax, %0\n\t"
							:"=r" (*operand)
							:"r" (*operand)
							:"%eax"
						);
	return;
}

 void atomic_decrement( int *operand ){

	__asm__ __volatile__(
							"mov %1, %%eax\n\t"
							"sub $0x01, %%eax\n\t"
							"xchg %%eax, %0\n\t"
							:"=r" (*operand)
							:"r" (*operand)
							:"%eax"
						);
	return;
}

 void atomic_modulus( int *operand, int *divisor ){

	__asm__ __volatile__(
							"mov %1, %%eax\n\t"
							"xor %%edx, %%edx\n\t"
							"mov %2, %%ecx\n\t"
							"idiv %%ecx\n\t"
							"mov %%edx, %0\n\t"
							:"=r"(*operand)
							:"r"(*operand), "r"(*divisor)
							:"%eax", "%ecx", "%edx"
						);
}
