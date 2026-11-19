/* { dg-do run } */
/* { dg-set-target-env-var GOMP_SPINCOUNT "10" } */
/* { dg-set-target-env-var GOMP_STACKSIZE "256" } */
/* { dg-set-target-env-var OMP_NUM_THREADS "4,3, " } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "true" } */
/* { dg-set-target-env-var OMP_PROC_BIND "true" } */
/* { dg-set-target-env-var OMP_PROC_BIND_ALL "master,master,close" } */
/* { dg-set-target-env-var OMP_PROC_BIND_DEV "master" } */
/* { dg-set-target-env-var OMP_SCHEDULE "guided,999999999999999999999" } */
/* { dg-set-target-env-var OMP_THREAD_LIMIT_ALL "5" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_NUM_THREADS: )" } */
/* { dg-output "(?=.*\\\[host] OMP_PROC_BIND = 'TRUE'.*\\\[all] OMP_PROC_BIND = 'MASTER,MASTER,CLOSE'.*\\\[device] OMP_PROC_BIND = 'MASTER')" } */
/* { dg-output "(?=.*libgomp: Invalid value for chunk size in environment variable OMP_SCHEDULE: 999999999999999999999)" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   GOMP_SPINCOUNT: a spin count below the throttled spin count.
   GOMP_STACKSIZE: GOMP_STACKSIZE without OMP_STACKSIZE.
   OMP_NUM_THREADS: a list which ends with a comma.
   OMP_PROC_BIND, OMP_PROC_BIND_ALL, OMP_PROC_BIND_DEV: the proc bind lists.
   OMP_SCHEDULE: a chunk size which overflows unsigned long.
   OMP_THREAD_LIMIT_ALL: the thread limit for all devices.  */

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

  if (omp_get_thread_limit () != 5)
    abort ();

  return 0;
}
