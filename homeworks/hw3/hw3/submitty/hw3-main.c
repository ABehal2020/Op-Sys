/* hw3-main.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int next_thread_id;        /* initialize to 1 */

int max_squares;           /* initialize to 0 */

int total_tours;           /* initialize to 0 */

/* write the simulate() function and place all of your code in hw3.c */
int simulate( int argc, char * argv[] );

int main( int argc, char * argv[] )
{
  next_thread_id = 1;
  max_squares = 0;
  total_tours = 0;
  int rc = simulate( argc, argv );

  /* on Submitty, there will be more code here that validates the
      global variables at this point... */

  return rc;
}

#if 0
  bash$ a.out 3 4 0 0

                      S...   (*** this diagram is incomplete ***)
                      ....
                      ....   (*** this diagram is one possibility ***)
                      /  \
            THREAD 1 /    \ THREAD 2
                    /      \
                S3..        S...
                ..1.        ....
                2...        .1..
                /  \            \
      THREAD 3 /    \ THREAD 4   \
              /      \            \
          S36.        S3..         etc.
          ..14        ..1.
          25..        2.4.
          /  \            \
THREAD 5 /    \ THREAD 6   \
        /      \            \
    S369        S369         etc.
    7a14        b814
    258b        25a7


     if we instead run hw3 code in NO_PARALLEL mode...


                      S...   (*** this diagram is incomplete ***)
                      ....
                      ....   (*** this diagram is the only possibility ***)
                      /  \                            ^^^^
            THREAD 1 /    \ THREAD 12
                    /      \
                S3..        S...
                ..1.        ....
                2...        .1..
                /  \            \
      THREAD 2 /    \ THREAD 5   \
              /      \            \           TO DO: verify that THREAD 12 is correct
          S36.        S3..         etc.
          ..14        ..1.                    TO DO: how many child threads do you end
          25..        2.4.                            up with (what is next_thread_id at
          /  \            \                            the end...)
THREAD 3 /    \ THREAD 4   \
        /      \            \
    S369        S369         etc.
    7a14        b814
    258b        25a7

#endif
