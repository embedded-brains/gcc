/* { dg-do run } */
/* { dg-set-target-env-var GOMP_SPINCOUNT "999999999999999999999" } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "true" } */
/* { dg-set-target-env-var OMP_ALLOCATOR "omp_thread_mem_alloc" } */
/* { dg-set-target-env-var OMP_MAX_TASK_PRIORITY "5" } */
/* { dg-set-target-env-var OMP_NUM_THREADS_ALL "3,2" } */
/* { dg-set-target-env-var OMP_SCHEDULE "guided,2 x" } */
/* { dg-set-target-env-var OMP_STACKSIZE_ALL "1m" } */
/* { dg-set-target-env-var OMP_THREAD_LIMIT "999999999999999999999" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable GOMP_SPINCOUNT)" } */
/* { dg-output "(?=.*\\\[host] OMP_ALLOCATOR = 'omp_thread_mem_alloc')" } */
/* { dg-output "(?=.*libgomp: Invalid value for chunk size in environment variable OMP_SCHEDULE: 2 x)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_THREAD_LIMIT: 999999999999999999999)" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   GOMP_SPINCOUNT: the invalid value '999999999999999999999'.
   OMP_ALLOCATOR: the allocator omp_thread_mem_alloc.
   OMP_MAX_TASK_PRIORITY: an int variable.
   OMP_NUM_THREADS_ALL: the number of threads for all devices.
   OMP_SCHEDULE: a chunk size with trailing characters.
   OMP_STACKSIZE_ALL: the stack size for all devices.
   OMP_THREAD_LIMIT: a number which overflows unsigned long.  */

#include <limits.h>
#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  if (omp_get_max_task_priority () != 5)
    abort ();

  /* A parallel make check may set OMP_NUM_THREADS, which takes
     precedence.  */
  if (getenv ("OMP_NUM_THREADS") == NULL && omp_get_max_threads () != 3)
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
