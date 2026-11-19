/* { dg-do run } */
/* { dg-set-target-env-var OMP_NUM_THREADS "4,3;2" } */
/* { dg-set-target-env-var OMP_SCHEDULE "guided,2147483648" } */
/* { dg-set-target-env-var OMP_DYNAMIC "maybe" } */
/* { dg-set-target-env-var OMP_WAIT_POLICY "lazy" } */
/* { dg-set-target-env-var GOMP_STACKSIZE "x" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_NUM_THREADS: 3;2)" } */
/* { dg-output "(?=.*libgomp: Invalid value for chunk size in environment variable OMP_SCHEDULE: 2147483648)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_DYNAMIC: X)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_WAIT_POLICY: X)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable GOMP_STACKSIZE: x)" } */

/* Test the name of the variable in the message about an invalid value.  The
   messages may come in any order.  The invalid part of the value follows the
   name.  OMP_DYNAMIC and OMP_WAIT_POLICY report an unknown value as X.  A
   chunk size larger than INT_MAX is invalid also where unsigned long has 32
   bits.  */

#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  omp_sched_t kind;
  int chunk;

  if (omp_get_dynamic () != 0)
    abort ();

  omp_get_schedule (&kind, &chunk);
  if (kind != omp_sched_dynamic || chunk != 1)
    abort ();

  return 0;
}
