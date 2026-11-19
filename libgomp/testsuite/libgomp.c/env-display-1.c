/* { dg-do run } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV " TRUE " } */
/* { dg-set-target-env-var OMP_SCHEDULE "monotonic:dynamic,3" } */
/* { dg-set-target-env-var OMP_SCHEDULE_ALL "nonmonotonic:static" } */
/* { dg-set-target-env-var OMP_SCHEDULE_DEV "auto" } */
/* { dg-set-target-env-var OMP_SCHEDULE_DEV_1 "static,5" } */
/* { dg-output ".*\\\[host] OMP_SCHEDULE = 'MONOTONIC:DYNAMIC,3'.*\\\[all] OMP_SCHEDULE = 'NONMONOTONIC:STATIC'.*\\\[device] OMP_SCHEDULE = 'AUTO'.*\\\[1] OMP_SCHEDULE = 'STATIC,5'.*\\\[host] OMP_MAX_ACTIVE_LEVELS = '1'\n  \\\[host] OMP_NUM_TEAMS.*" } */

/* Test the display of the ICVs of a device.  OMP_MAX_ACTIVE_LEVELS appears
   only for the devices which set it.  */

int
main (void)
{
  return 0;
}
