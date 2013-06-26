      #include "gtthread.h"
	
      #include <stdio.h>
      #include <unistd.h>
      #include <stdlib.h>
      #include <errno.h>
      #include <assert.h>

      #define PHILOS 5
      #define DELAY 5000
      #define FOOD 50

    	void *philosopher (void *id);
    	void grab_chopstick (int,
    	                     int,
                         char *);
    	void down_chopsticks (int,
    	                      int);
    	int food_on_table ();

    	gtthread_mutex_t chopstick[PHILOS];
    	gtthread_t philo[PHILOS];
    	gtthread_mutex_t food_lock;
    	int sleep_seconds = 0;


    	int
    	main (int argn,
   	      char **argv)
    	{
    	    int i;
    	    gtthread_init( 10000 );
    	    if (argn == 2)
    	        sleep_seconds = atoi (argv[1]);

    	    gtthread_mutex_init (&food_lock);
    	    for (i = 0; i < PHILOS; i++)
    	        gtthread_mutex_init (&chopstick[i]);
    	    for (i = 0; i < PHILOS; i++)
    	        gtthread_create (&philo[i], philosopher, (void *)i);
    	    for (i = 0; i < PHILOS; i++)
    	        gtthread_join (philo[i], NULL);
    	    return 0;
    	}

    	void *
    	philosopher (void *num)
    	{
    	    int id;
    	    int i, left_chopstick, right_chopstick, f;

    	    id = (intptr_t)num;
    	    printf ("Philosopher %d is done thinking and now ready to eat.\n", id);
    	    right_chopstick = id;
    	    left_chopstick = id + 1;

    	    /* Wrap around the chopsticks. */
    	    if (left_chopstick == PHILOS)
    	        left_chopstick = 0;

    	    while (f = food_on_table ()) {

    	        /* Thanks to philosophers #1 who would like to take a nap
    	         * before picking up the chopsticks, the other philosophers
    	         * may be able to eat their dishes and not deadlock.
    	         */
    	        if (id == 1)
    	            sleep (sleep_seconds);

    	        grab_chopstick (id, right_chopstick, "right ");
    	        grab_chopstick (id, left_chopstick, "left");

    	        printf ("Philosopher %d: eating.\n", id);
    	        usleep (DELAY * (FOOD - f + 1));
    	        down_chopsticks (left_chopstick, right_chopstick);
    	    }

    	    printf ("Philosopher %d is done eating.\n", id);
    	    return (NULL);
    	}

    	int
    	food_on_table ()
    	{
        static int food = FOOD;
    	    int myfood;

    	    gtthread_mutex_lock (&food_lock);
    	    if (food > 0) {
    	        food--;
    	    }
    	    myfood = food;
    	    gtthread_mutex_unlock (&food_lock);
    	    return myfood;
    	}

    	void
    	grab_chopstick (int phil,
    	                int c,
    	                char *hand)
    	{
    		gtthread_mutex_lock (&chopstick[c]);
    		printf ("Philosopher %d: got %s chopstick %d\n", phil, hand, c);
    	}

    	void
    	down_chopsticks (int c1,
   	                 int c2)
    	{
    		gtthread_mutex_unlock (&chopstick[c1]);
    		gtthread_mutex_unlock (&chopstick[c2]);
    	}

