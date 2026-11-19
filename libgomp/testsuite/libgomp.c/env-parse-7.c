/* { dg-do run } */
/* { dg-set-target-env-var GOMP_DEBUG "0" } */
/* { dg-set-target-env-var GOMP_SPINCOUNT "2 M" } */
/* { dg-set-target-env-var OMP_AFFINITY_FORMAT "T%n" } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "maybe" } */
/* { dg-set-target-env-var OMP_DYNAMIC_DEV "true" } */
/* { dg-set-target-env-var OMP_NESTED "true" } */
/* { dg-set-target-env-var OMP_NUM_THREADS "4;3" } */
/* { dg-set-target-env-var OMP_SCHEDULE "auto" } */
/* { dg-set-target-env-var OMP_STACKSIZE "4t" } */
/* { dg-set-target-env-var OMP_WAIT_POLICY_ALL "passive" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_DISPLAY_ENV)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_NUM_THREADS: 4;3)" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_STACKSIZE: 4t)" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   GOMP_DEBUG: a value of zero.
   GOMP_SPINCOUNT: the value 2 M.
   OMP_AFFINITY_FORMAT: a format with a field.
   OMP_DISPLAY_ENV: an unknown value of OMP_DISPLAY_ENV.
   OMP_DYNAMIC_DEV: a value for the devices.
   OMP_NESTED: the value true.
   OMP_NUM_THREADS: a first value with trailing characters.
   OMP_SCHEDULE: the auto kind.
   OMP_STACKSIZE: an unknown unit.
   OMP_WAIT_POLICY_ALL: the wait policy for all devices.  */

#include <omp.h>
#include <stdlib.h>

int
main (void)
{
  {
    char buf[8];

    if (omp_get_affinity_format (buf, sizeof (buf)) != 3
	|| __builtin_strcmp (buf, "T%n") != 0)
      abort ();
  }

  if (omp_get_dynamic () != 0)
    abort ();

  if (omp_get_max_active_levels () != omp_get_supported_active_levels ())
    abort ();

  {
    omp_sched_t kind;
    int chunk;

    omp_get_schedule (&kind, &chunk);
    if (kind != omp_sched_auto || chunk != 1)
      abort ();
  }

  return 0;
}
