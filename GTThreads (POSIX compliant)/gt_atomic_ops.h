#ifndef __GT_ATOMIC_OPS__
#define __GT_ATOMIC_OPS__
#include "gtthread.h"
#include "gt_mutex.h"
#include "gt_signals.h"
#include "gt_atomic_ops.h"
#include "gt_context.h"
//Atomic operations
extern void atomic_increment( int *operand );
extern void atomic_decrement( int *operand );
extern void atomic_modulus( int *operand, int *divisor );

#endif
