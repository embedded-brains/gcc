/* { dg-do run } */
/* { dg-set-target-env-var GOMP_SPINCOUNT " " } */
/* { dg-set-target-env-var OMP_DYNAMIC_FOO "true" } */
/* { dg-set-target-env-var OMP_DYNAMIC_DEV_01 "true" } */
/* { dg-set-target-env-var OMP_DYNAMIC_DEV_x "true" } */
/* { dg-set-target-env-var OMP_DYNAMIC_DEV_2147483648 "true" } */
/* { dg-set-target-env-var OMP_CANCELLATION_ALL "true" } */
/* { dg-set-target-env-var OMP_CANCELLATION_DEV "true" } */
/* { dg-set-target-env-var OMP_CANCELLATION_DEV_0 "true" } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "true x" } */
/* { dg-set-target-env-var OMP_NUM_THREADS "4 ,3" } */
/* { dg-set-target-env-var OMP_SCHEDULE "fast" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable GOMP_SPINCOUNT)" } */
/* { dg-output "(?=.*libgomp: Invalid environment variable in OMP_DYNAMIC_FOO=true)" } */
/* { dg-output "(?=.*libgomp: Invalid device number in OMP_DYNAMIC_DEV_01=true)" } */
/* { dg-output "(?=.*libgomp: Invalid device number in OMP_DYNAMIC_DEV_x=true)" } */
/* { dg-output "(?=.*libgomp: Invalid device number in OMP_DYNAMIC_DEV_2147483648=true)" } */
/* { dg-output "(?=.*libgomp: Invalid environment variable in OMP_CANCELLATION_ALL=true)" } */
/* { dg-output "(?=.*libgomp: Invalid environment variable in OMP_CANCELLATION_DEV=true)" } */
/* { dg-output "(?=.*libgomp: Invalid environment variable in OMP_CANCELLATION_DEV_0=true)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_DISPLAY_ENV.*OPENMP DISPLAY ENVIRONMENT BEGIN)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_SCHEDULE: fast)" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   GOMP_SPINCOUNT: the invalid value ' '.
   OMP_DYNAMIC_FOO: an unknown suffix.
   OMP_DYNAMIC_DEV_01: a device number with a leading zero.
   OMP_DYNAMIC_DEV_x: a device number which is no number.
   OMP_DYNAMIC_DEV_2147483648: a device number larger than INT_MAX.
   OMP_CANCELLATION_ALL: a suffix for a variable without device suffixes.
   OMP_CANCELLATION_DEV: a device suffix for a variable without device
     suffixes.
   OMP_CANCELLATION_DEV_0: a device number for a variable without device
     suffixes.
   OMP_DISPLAY_ENV: a value of OMP_DISPLAY_ENV with trailing characters.
   OMP_NUM_THREADS: a blank before the first comma.
   OMP_SCHEDULE: an unknown kind.  */

#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  if (omp_get_dynamic () != 0)
    abort ();

  if (omp_get_cancellation ())
    abort ();

  if (omp_get_max_threads () != 4)
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
