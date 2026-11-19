/* { dg-do run } */
/* { dg-set-target-env-var GOMP_SPINCOUNT "INFINITY" } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "true" } */
/* { dg-set-target-env-var OMP_ALLOCATOR "omp_const_mem_alloc" } */
/* { dg-set-target-env-var OMP_DYNAMIC_DEV_0 "true" } */
/* { dg-set-target-env-var OMP_NESTED "false" } */
/* { dg-set-target-env-var OMP_NUM_THREADS "4,0" } */
/* { dg-set-target-env-var OMP_SCHEDULE "NONMONOTONIC:guided" } */
/* { dg-set-target-env-var OMP_STACKSIZE " " } */
/* { dg-set-target-env-var OMP_WAIT_POLICY "active x" } */
/* { dg-output "(?=.*\\\[host] OMP_ALLOCATOR = 'omp_const_mem_alloc')" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_NUM_THREADS: 0)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_STACKSIZE: )" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_WAIT_POLICY: x)" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   GOMP_SPINCOUNT: the value infinity.
   OMP_ALLOCATOR: the allocator omp_const_mem_alloc.
   OMP_DYNAMIC_DEV_0: a value for a device.
   OMP_NESTED: the value false.
   OMP_NUM_THREADS: a list element of zero.
   OMP_SCHEDULE: a nonmonotonic modifier without a chunk size.
   OMP_STACKSIZE: a value of only blanks.
   OMP_WAIT_POLICY: a wait policy with trailing characters.  */

#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  if (omp_get_dynamic () != 0)
    abort ();

  if (omp_get_max_active_levels () != 1)
    abort ();

  {
    omp_sched_t kind;
    int chunk;

    omp_get_schedule (&kind, &chunk);
    if (kind != omp_sched_guided || chunk != 1)
      abort ();
  }

  return 0;
}
