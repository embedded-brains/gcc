/* { dg-do run } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "true" } */
/* { dg-set-target-env-var OMP_NUM_TEAMS_DEV_2 "4" } */
/* { dg-set-target-env-var OMP_DYNAMIC_DEV_2 "true" } */
/* { dg-set-target-env-var OMP_TEAMS_THREAD_LIMIT_DEV_2 "5" } */
/* { dg-set-target-env-var OMP_SCHEDULE_DEV_2 "guided" } */
/* { dg-set-target-env-var OMP_THREAD_LIMIT_DEV_2 "6" } */
/* { dg-set-target-env-var OMP_NUM_THREADS_DEV_2 "3,2" } */
/* { dg-set-target-env-var OMP_PROC_BIND_DEV_2 "spread" } */
/* { dg-set-target-env-var OMP_WAIT_POLICY_DEV_2 "active" } */
/* { dg-set-target-env-var OMP_STACKSIZE_DEV_2 "1m" } */
/* { dg-set-target-env-var OMP_MAX_ACTIVE_LEVELS_DEV_2 "2" } */
/* { dg-set-target-env-var OMP_SCHEDULE "guided, " } */
/* { dg-output "(?=.*\\\[2] OMP_DYNAMIC = 'TRUE'.*\\\[2] OMP_NUM_THREADS = '3,2'.*\\\[2] OMP_SCHEDULE = 'GUIDED'.*\\\[2] OMP_PROC_BIND = 'SPREAD'.*\\\[2] OMP_STACKSIZE = '1048576'.*\\\[2] OMP_WAIT_POLICY = 'ACTIVE'.*\\\[2] OMP_THREAD_LIMIT = '6'.*\\\[2] OMP_MAX_ACTIVE_LEVELS = '2'.*\\\[2] OMP_NUM_TEAMS = '4'.*\\\[2] OMP_TEAMS_THREAD_LIMIT = '5')" } */
/* { dg-output "(?=.*libgomp: Invalid value for chunk size in environment variable OMP_SCHEDULE: )" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   OMP_NUM_TEAMS_DEV_2, OMP_DYNAMIC_DEV_2, OMP_TEAMS_THREAD_LIMIT_DEV_2,
     OMP_SCHEDULE_DEV_2, OMP_THREAD_LIMIT_DEV_2, OMP_NUM_THREADS_DEV_2,
     OMP_PROC_BIND_DEV_2, OMP_WAIT_POLICY_DEV_2, OMP_STACKSIZE_DEV_2,
     OMP_MAX_ACTIVE_LEVELS_DEV_2: the ICVs of a device.
   OMP_SCHEDULE: a comma without a chunk size.  */

#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  {
    omp_sched_t kind;
    int chunk;

    omp_get_schedule (&kind, &chunk);
    if (kind != omp_sched_dynamic || chunk != 1)
      abort ();
  }

  return 0;
}
