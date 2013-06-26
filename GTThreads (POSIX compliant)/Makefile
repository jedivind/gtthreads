#### GTThread Library Makefile

CFLAGS  = ## -Wall -pedantic
LFLAGS  =
CC      = gcc
RM      = /bin/rm -rf

LIB_SRC = gtthread.c 
LIB_MUTEX = gt_mutex.c 
LIB_CONTEXT = gt_context.c 
LIB_SIGNALS = gt_signals.c 
LIB_ATOMIC = gt_atomic_ops.c

LIB_OBJ = gt_mutex.o gt_atomic_ops.o gt_signals.o gt_context.o gtthread.o

# pattern rule for object files
%.o: %.c
	        $(CC) -c $(CFLAGS) $< -o $@

all: $(LIB_OBJ)

$(LIB_OBJ) : $(LIB_SRC) $(LIB_MUTEX) $(LIB_CONTEXT) $(LIB_SIGNALS) $(LIB_ATOMIC)
	$(CC) -c $(CFLAGS) gtthread.c -o gtthread.o
	$(CC) -c $(CFLAGS) gt_atomic_ops.c -o gt_atomic_ops.o 
	$(CC) -c $(CFLAGS) gt_signals.c -o gt_signals.o
	$(CC) -c $(CFLAGS) gt_context.c -o gt_context.o
	$(CC) -c $(CFLAGS) gt_mutex.c -o gt_mutex.o

$(LIB_SRC):

clean:
	        $(RM) $(LIB_OBJ) 

.PHONY: depend
depend:
	        makedepend -Y -- $(CFLAGS) -- $(LIB_SRC)  2>/dev/null
