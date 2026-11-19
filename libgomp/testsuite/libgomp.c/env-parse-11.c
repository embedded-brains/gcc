/* { dg-do run } */
/* { dg-set-target-env-var GOMP_SPINCOUNT "20000000t" } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "true" } */
/* { dg-set-target-env-var OMP_ALLOCATOR "omp_pteam_mem_alloc" } */
/* { dg-set-target-env-var OMP_DYNAMIC_ALL "true" } */
/* { dg-set-target-env-var OMP_DYNAMIC "false" } */
/* { dg-set-target-env-var OMP_NUM_THREADS " " } */
/* { dg-set-target-env-var OMP_SCHEDULE "monotonic dynamic" } */
/* { dg-set-target-env-var OMP_THREAD_LIMIT "0" } */
/* { dg-output "(?=.*\\\[host] OMP_ALLOCATOR = 'omp_pteam_mem_alloc')" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_NUM_THREADS: )" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_SCHEDULE: dynamic)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_THREAD_LIMIT: 0)" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   GOMP_SPINCOUNT: the value 20000000t.
   OMP_ALLOCATOR: the allocator omp_pteam_mem_alloc.
   OMP_DYNAMIC_ALL, OMP_DYNAMIC: a value without a suffix over the value for
     all devices.
   OMP_NUM_THREADS: a value of only blanks.
   OMP_SCHEDULE: a modifier without a colon.
   OMP_THREAD_LIMIT: a value of zero.  */

#include <limits.h>
#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  if (omp_get_dynamic () != 0)
    abort ();

  {
    omp_sched_t kind;
    int chunk;

    omp_get_schedule (&kind, &chunk);
    if (kind != omp_sched_dynamic || chunk != 1)
      abort ();
  }

  if (omp_get_thread_limit () != INT_MAX)
    abort ();

  return 0;
}
