/* { dg-do run } */
/* { dg-set-target-env-var OMP_DISPLAY_ENV "verbose" } */
/* { dg-set-target-env-var OMP_DYNAMIC_ALL "true" } */
/* { dg-set-target-env-var OMP_DYNAMIC_DEV "true" } */
/* { dg-set-target-env-var OMP_WAIT_POLICY_ALL "active" } */
/* { dg-set-target-env-var OMP_WAIT_POLICY_DEV "passive" } */
/* { dg-set-target-env-var OMP_WAIT_POLICY "passive" } */
/* { dg-set-target-env-var OMP_STACKSIZE "64k" } */
/* { dg-set-target-env-var OMP_STACKSIZE_ALL "128k" } */
/* { dg-set-target-env-var OMP_THREAD_LIMIT "3" } */
/* { dg-set-target-env-var OMP_THREAD_LIMIT_ALL "4" } */
/* { dg-set-target-env-var OMP_MAX_ACTIVE_LEVELS "2" } */
/* { dg-set-target-env-var OMP_MAX_ACTIVE_LEVELS_ALL "3" } */
/* { dg-set-target-env-var OMP_NUM_TEAMS "2" } */
/* { dg-set-target-env-var OMP_NUM_TEAMS_ALL "3" } */
/* { dg-set-target-env-var OMP_TEAMS_THREAD_LIMIT "2" } */
/* { dg-set-target-env-var OMP_TEAMS_THREAD_LIMIT_ALL "3" } */
/* { dg-output "(?=.*\\\[all] OMP_DYNAMIC = 'TRUE'.*\\\[device] OMP_DYNAMIC = 'TRUE'.*\\\[all] OMP_STACKSIZE = '131072'.*\\\[host] OMP_WAIT_POLICY = 'PASSIVE'.*\\\[all] OMP_WAIT_POLICY = 'ACTIVE'.*\\\[device] OMP_WAIT_POLICY = 'PASSIVE'.*\\\[host] OMP_THREAD_LIMIT = '3'.*\\\[all] OMP_THREAD_LIMIT = '4'.*\\\[host] OMP_MAX_ACTIVE_LEVELS = '2'.*\\\[all] OMP_MAX_ACTIVE_LEVELS = '3'.*\\\[host] OMP_NUM_TEAMS = '2'.*\\\[all] OMP_NUM_TEAMS = '3'.*\\\[host] OMP_TEAMS_THREAD_LIMIT = '2'.*\\\[all] OMP_TEAMS_THREAD_LIMIT = '3'.*\\\[host] GOMP_STACKSIZE = '65536')" } */

/* Test the parsing and the display of the environment.  libgomp reads the
   environment once at startup, so a test takes at most one case of each ICV.
   The messages may come in any order.  The cases are:
   OMP_DYNAMIC_ALL, OMP_DYNAMIC_DEV, OMP_WAIT_POLICY_ALL, OMP_WAIT_POLICY_DEV,
     OMP_WAIT_POLICY, OMP_STACKSIZE, OMP_STACKSIZE_ALL, OMP_THREAD_LIMIT,
     OMP_THREAD_LIMIT_ALL, OMP_MAX_ACTIVE_LEVELS, OMP_MAX_ACTIVE_LEVELS_ALL,
     OMP_NUM_TEAMS, OMP_NUM_TEAMS_ALL, OMP_TEAMS_THREAD_LIMIT,
     OMP_TEAMS_THREAD_LIMIT_ALL: the ICVs for all devices and for the devices.  */

int
main (void)
{
  return 0;
}
