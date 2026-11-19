/* { dg-do run } */
/* { dg-set-target-env-var GOMP_SPINCOUNT "5q" } */
/* { dg-set-target-env-var OMP_NUM_THREADS "2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1" } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "true" } */
/* { dg-set-target-env-var OMP_SCHEDULE "static,4" } */
/* { dg-set-target-env-var OMP_SCHEDULE_ALL "guided,7" } */
/* { dg-set-target-env-var OMP_STACKSIZE "1 B" } */
/* { dg-set-target-env-var OMP_THREAD_LIMIT " 7 " } */
/* { dg-set-target-env-var OMP_WAIT_POLICY " passive " } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable GOMP_SPINCOUNT)" } */
/* { dg-output "(?=.*\\\[host] OMP_SCHEDULE = 'STATIC,4'.*\\\[all] OMP_SCHEDULE = 'GUIDED,7')" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   GOMP_SPINCOUNT: the invalid value '5q'.
   OMP_NUM_THREADS: a list of seventeen values.
   OMP_SCHEDULE, OMP_SCHEDULE_ALL: a static schedule with a chunk size and a
     guided schedule.
   OMP_STACKSIZE: a size in bytes.
   OMP_THREAD_LIMIT: blanks around a number.
   OMP_WAIT_POLICY: a passive wait policy with blanks.  */

#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  if (omp_get_max_threads () != 2)
    abort ();

  if (omp_get_thread_limit () != 7)
    abort ();

  return 0;
}
