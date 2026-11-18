/* { dg-do run } */
/* { dg-set-target-env-var OMP_CANCELLATION "true" } */

/* Test the constructs which run after the cancellation of their
   taskgroup, the children which a cancelled taskgroup discards, the
   cancellation of the taskgroup of a worksharing loop with a task
   reduction, and a sections construct with a barrier after its
   cancellation.  */

#include <omp.h>
#include <stdlib.h>

static int bad;
static int ran;
static int sum;

__attribute__((noipa)) void
after_cancel (void)
{
  int x = 0;

  #pragma omp taskgroup
  {
    long i;
    unsigned long long u;

    #pragma omp task
    {
      #pragma omp cancel taskgroup
    }
    #pragma omp taskwait

    /* The taskgroup is cancelled, so none of these tasks runs.  */
    #pragma omp task
    bad = 1;

    #pragma omp taskloop nogroup
    for (i = 0; i < 10; i++)
      bad = 1;

    #pragma omp taskloop nogroup
    for (u = 0; u < 10; u++)
      bad = 1;

    #pragma omp taskwait depend (in: x)
  }
}

__attribute__((noipa)) void
discard_children (void)
{
  #pragma omp taskgroup
  {
    int i;

    #pragma omp task
    {
      #pragma omp cancel taskgroup
    }
    for (i = 0; i < 20; i++)
      #pragma omp task
      {
	#pragma omp atomic
	ran++;
      }
  }
}

__attribute__((noipa)) void
workshare_taskgroup (void)
{
  int i;

  #pragma omp for reduction (task, +: sum)
  for (i = 0; i < 8; i++)
    {
      int x = 0;

      /* The undeferred task reaches its cancellation point before the
	 taskgroup is cancelled in the first iteration, and possibly after it
	 in a later one.  */
      #pragma omp task if (0) in_reduction (+: sum)
      {
	#pragma omp cancellation point taskgroup
	sum++;
      }
      #pragma omp task in_reduction (+: sum)
      {
	sum++;
	#pragma omp cancel taskgroup
      }
      #pragma omp taskwait depend (in: x)
    }
}

__attribute__((noipa)) int
sections (void)
{
  int a = 0, b = 0;

  #pragma omp parallel num_threads (2) reduction (+: b)
  {
    #pragma omp sections
    {
      #pragma omp section
      {
	#pragma omp cancel sections
	a = 1;
      }
      #pragma omp section
      {
	#pragma omp cancel sections
	a = 1;
      }
    }
    /* The statement keeps the barrier at the end of the sections.  The
       cancel construct makes the parallel region cancellable, so this
       barrier is a cancellable one.  Its condition is false, so nothing
       cancels the parallel region.  */
    b++;
    #pragma omp cancel parallel if (a != 0)
  }
  return a ? -1 : b;
}

int
main (void)
{
  if (!omp_get_cancellation ())
    return 0;

  #pragma omp parallel num_threads (2)
  {
    #pragma omp single
    {
      after_cancel ();
      discard_children ();
    }
    workshare_taskgroup ();
  }
  if (bad || ran > 20 || sum < 0 || sum > 16)
    abort ();
  if (sections () != 2)
    abort ();
  return 0;
}
