/* { dg-do run } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "false" } */
/* { dg-set-target-env-var OMP_DYNAMIC " TRUE " } */
/* { dg-set-target-env-var OMP_MAX_ACTIVE_LEVELS "1000000" } */
/* { dg-set-target-env-var OMP_NUM_TEAMS_ALL "3" } */
/* { dg-set-target-env-var OMP_NUM_THREADS "0" } */
/* { dg-set-target-env-var OMP_SCHEDULE "static,0" } */
/* { dg-set-target-env-var OMP_STACKSIZE "17179869184g" } */
/* { dg-set-target-env-var OMP_THREAD_LIMIT "  " } */
/* { dg-set-target-env-var OMP_WAIT_POLICY "ACTIVE" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_NUM_THREADS: 0)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_STACKSIZE: 17179869184g)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_THREAD_LIMIT: )" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   OMP_DISPLAY_ENV: the value false.
   OMP_DYNAMIC: a true value with blanks.
   OMP_MAX_ACTIVE_LEVELS: a value above the supported levels.
   OMP_NUM_TEAMS_ALL: an int variable for all devices.
   OMP_NUM_THREADS: a first value of zero.
   OMP_SCHEDULE: the static kind with a chunk size of zero.
   OMP_STACKSIZE: a size which overflows in bytes.
   OMP_THREAD_LIMIT: a value of only blanks.
   OMP_WAIT_POLICY: an active wait policy.  */

#include <limits.h>
#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  if (omp_get_dynamic () != 1)
    abort ();

  if (omp_get_max_active_levels () != omp_get_supported_active_levels ())
    abort ();

  if (omp_get_max_teams () != 3)
    abort ();

  {
    omp_sched_t kind;
    int chunk;

    omp_get_schedule (&kind, &chunk);
    if (kind != (omp_sched_static | omp_sched_monotonic) || chunk != 0)
      abort ();
  }

  if (omp_get_thread_limit () != INT_MAX)
    abort ();

  return 0;
}
