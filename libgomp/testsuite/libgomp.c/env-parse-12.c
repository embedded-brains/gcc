/* { dg-do run } */
/* { dg-set-target-env-var GOMP_SPINCOUNT "infinitex" } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "true" } */
/* { dg-set-target-env-var OMP_ALLOCATOR "omp_low_lat_mem_alloc" } */
/* { dg-set-target-env-var OMP_MAX_ACTIVE_LEVELS_ALL "3" } */
/* { dg-set-target-env-var OMP_NUM_THREADS "4,999999999999999999999" } */
/* { dg-set-target-env-var OMP_SCHEDULE "guided,x" } */
/* { dg-set-target-env-var OMP_STACKSIZE "4194304g" } */
/* { dg-set-target-env-var OMP_THREAD_LIMIT "3 x" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable GOMP_SPINCOUNT)" } */
/* { dg-output "(?=.*\\\[host] OMP_ALLOCATOR = 'omp_low_lat_mem_alloc')" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_NUM_THREADS: 999999999999999999999)" } */
/* { dg-output "(?=.*libgomp: Invalid value for chunk size in environment variable OMP_SCHEDULE: x)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_STACKSIZE: 4194304g)" { target ilp32 } } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_THREAD_LIMIT: 3 x)" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   GOMP_SPINCOUNT: the invalid value 'infinitex'.
   OMP_ALLOCATOR: the allocator omp_low_lat_mem_alloc.
   OMP_MAX_ACTIVE_LEVELS_ALL: the active levels for all devices.
   OMP_NUM_THREADS: a list element which overflows unsigned long.
   OMP_SCHEDULE: a chunk size which is no number.
   OMP_STACKSIZE: a size which overflows in bytes on a 32-bit target.
   OMP_THREAD_LIMIT: a number with trailing characters.  */

#include <limits.h>
#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  if (omp_get_max_active_levels () != 3)
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
