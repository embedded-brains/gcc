/* { dg-do run } */
/* { dg-set-target-env-var GOMP_SPINCOUNT "3g" } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "true" } */
/* { dg-set-target-env-var OMP_ALLOCATOR "omp_high_bw_mem_alloc" } */
/* { dg-set-target-env-var OMP_DYNAMIC "true x" } */
/* { dg-set-target-env-var OMP_NUM_THREADS "4, 3 ,2" } */
/* { dg-set-target-env-var OMP_PROC_BIND "false" } */
/* { dg-set-target-env-var OMP_SCHEDULE "dynamic,0" } */
/* { dg-set-target-env-var OMP_STACKSIZE "4k x" } */
/* { dg-output "(?=.*\\\[host] OMP_ALLOCATOR = 'omp_high_bw_mem_alloc')" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_DYNAMIC: x)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_STACKSIZE: 4k x)" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   GOMP_SPINCOUNT: the value 3g.
   OMP_ALLOCATOR: the allocator omp_high_bw_mem_alloc.
   OMP_DYNAMIC: a value with trailing characters.
   OMP_NUM_THREADS: a list of three values.
   OMP_PROC_BIND: the value false.
   OMP_SCHEDULE: a chunk size of zero for a dynamic schedule.
   OMP_STACKSIZE: a unit with trailing characters.  */

#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  if (omp_get_dynamic () != 0)
    abort ();

  if (omp_get_max_threads () != 4)
    abort ();
  #pragma omp parallel num_threads (1)
  if (omp_get_max_threads () != 3)
    abort ();

  if (omp_get_proc_bind () != omp_proc_bind_false)
    abort ();

  {
    omp_sched_t kind;
    int chunk;

    omp_get_schedule (&kind, &chunk);
    if (kind != omp_sched_dynamic || chunk != 1)
      abort ();
  }

  return 0;
}
