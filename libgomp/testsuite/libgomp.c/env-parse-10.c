/* { dg-do run } */
/* { dg-set-target-env-var GOMP_SPINCOUNT "5k x" } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "true" } */
/* { dg-set-target-env-var OMP_ALLOCATOR "omp_cgroup_mem_alloc" } */
/* { dg-set-target-env-var OMP_DYNAMIC_ALL "true" } */
/* { dg-set-target-env-var OMP_NUM_THREADS "999999999999999999999" } */
/* { dg-set-target-env-var OMP_SCHEDULE " monotonic : dynamic , 3 " } */
/* { dg-set-target-env-var OMP_STACKSIZE "999999999999999999999" } */
/* { dg-set-target-env-var OMP_THREAD_LIMIT "many" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable GOMP_SPINCOUNT)" } */
/* { dg-output "(?=.*\\\[host] OMP_ALLOCATOR = 'omp_cgroup_mem_alloc')" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_NUM_THREADS: 999999999999999999999)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_STACKSIZE: 999999999999999999999)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_THREAD_LIMIT: many)" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   GOMP_SPINCOUNT: the invalid value '5k x'.
   OMP_ALLOCATOR: the allocator omp_cgroup_mem_alloc.
   OMP_DYNAMIC_ALL: the value for all devices.
   OMP_NUM_THREADS: a first value which overflows unsigned long.
   OMP_SCHEDULE: a monotonic modifier, blanks and a chunk size.
   OMP_STACKSIZE: a number which overflows unsigned long.
   OMP_THREAD_LIMIT: a value which is no number.  */

#include <limits.h>
#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  if (omp_get_dynamic () != 1)
    abort ();

  {
    omp_sched_t kind;
    int chunk;

    omp_get_schedule (&kind, &chunk);
    if (kind != (omp_sched_dynamic | omp_sched_monotonic) || chunk != 3)
      abort ();
  }

  if (omp_get_thread_limit () != INT_MAX)
    abort ();

  return 0;
}
