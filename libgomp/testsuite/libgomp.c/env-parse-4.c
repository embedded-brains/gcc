/* { dg-do run } */
/* { dg-set-target-env-var GOMP_SPINCOUNT "fast" } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "true" } */
/* { dg-set-target-env-var OMP_DYNAMIC_ALL "false" } */
/* { dg-set-target-env-var OMP_DYNAMIC_DEV "false" } */
/* { dg-set-target-env-var OMP_DYNAMIC_DEV_3 "false" } */
/* { dg-set-target-env-var OMP_WAIT_POLICY_ALL "passive" } */
/* { dg-set-target-env-var OMP_WAIT_POLICY_DEV "active" } */
/* { dg-set-target-env-var OMP_PROC_BIND "spread,close,primary" } */
/* { dg-set-target-env-var OMP_TARGET_OFFLOAD "disabled" } */
/* { dg-set-target-env-var OMP_SCHEDULE "guided;2" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable GOMP_SPINCOUNT)" } */
/* { dg-output "(?=.*\\\[all] OMP_DYNAMIC = 'FALSE'.*\\\[device] OMP_DYNAMIC = 'FALSE'.*\\\[3] OMP_DYNAMIC = 'FALSE'.*\\\[host] OMP_PROC_BIND = 'SPREAD,CLOSE,MASTER'.*\\\[all] OMP_WAIT_POLICY = 'PASSIVE'.*\\\[device] OMP_WAIT_POLICY = 'ACTIVE'.*\\\[all] OMP_TARGET_OFFLOAD = 'DISABLED')" } */
/* { dg-output "(?=.*libgomp: Invalid value for environment variable OMP_SCHEDULE: ;?2)" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   GOMP_SPINCOUNT: the invalid value 'fast'.
   OMP_DYNAMIC_ALL, OMP_DYNAMIC_DEV, OMP_DYNAMIC_DEV_3, OMP_WAIT_POLICY_ALL,
     OMP_WAIT_POLICY_DEV, OMP_PROC_BIND, OMP_TARGET_OFFLOAD: the false and
     passive values of all devices and of the devices, a proc bind list and a
     disabled target offload.
   OMP_SCHEDULE: a separator other than a comma.  */

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

  return 0;
}
