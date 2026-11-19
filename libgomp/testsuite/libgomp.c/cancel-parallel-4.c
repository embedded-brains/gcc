/* { dg-do run } */
/* { dg-set-target-env-var OMP_CANCELLATION "true" } */
/* { dg-set-target-env-var OMP_WAIT_POLICY "passive" } */

/* Test that the threads which wait in the barrier at the end of a cancelled
   parallel region block until the last thread arrives.  */

#include <omp.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int
main ()
{
  if (!omp_get_cancellation ())
    return 0;

  omp_set_dynamic (0);
  #pragma omp parallel num_threads (4)
  {
    if (omp_get_thread_num () == 0)
      {
	/* Let threads 2 and 3 arrive at the barrier first.  */
	sleep (1);
	#pragma omp cancel parallel
      }
    else if (omp_get_thread_num () == 1)
      sleep (2);
  }

#if defined _POSIX_THREAD_CPUTIME && _POSIX_THREAD_CPUTIME >= 0
  /* The team reuses the threads of the cancelled parallel region.  A thread
     which spun in the barrier used about one second of processor time.  */
  #pragma omp parallel num_threads (4)
  {
    struct timespec ts;

    if (clock_gettime (CLOCK_THREAD_CPUTIME_ID, &ts) == 0
	&& (ts.tv_sec > 0 || ts.tv_nsec > 500000000))
      abort ();
  }
#endif
  return 0;
}
