/* { dg-do run } */
/* { dg-set-target-env-var OMP_MAX_TASK_PRIORITY "2" } */

/* Test taskloop with an unsigned long long iteration variable: upward and
   downward loops, empty loops, grainsize and num_tasks with and without
   strict, nogroup, if (0), final, priority and firstprivate of a
   variable length array.  Test the undeferred tasks of taskloop: inside a
   final task, with more tasks than the team may queue, with child tasks, and
   with firstprivate of a variable length array and a last task of another
   size.  */

#include <omp.h>
#include <stdlib.h>

#define N 64

static int hit[N];
static int child[N];

static void
check (unsigned long long start, unsigned long long end,
       unsigned long long step)
{
  unsigned long long i;

  for (i = 0; i < N; i++)
    {
      int expected = 0;

      if (step > 0 && i >= start && i < end && (i - start) % step == 0)
	expected = 1;
      if (hit[i] != expected)
	abort ();
      hit[i] = 0;
    }
}

static void
check_down (unsigned long long start, unsigned long long end,
	    unsigned long long step)
{
  unsigned long long i;

  for (i = 0; i < N; i++)
    {
      int expected = 0;

      if (i <= start && i > end && (start - i) % step == 0)
	expected = 1;
      if (hit[i] != expected)
	abort ();
      hit[i] = 0;
    }
}

__attribute__((noipa)) void
up (unsigned long long start, unsigned long long end,
    unsigned long long step, int kind, unsigned long long arg)
{
  unsigned long long i;

  switch (kind)
    {
    case 0:
      #pragma omp taskloop
      for (i = start; i < end; i += step)
	hit[i]++;
      break;
    case 1:
      #pragma omp taskloop grainsize (arg)
      for (i = start; i < end; i += step)
	hit[i]++;
      break;
    case 2:
      #pragma omp taskloop grainsize (strict: arg)
      for (i = start; i < end; i += step)
	hit[i]++;
      break;
    case 3:
      #pragma omp taskloop num_tasks (arg)
      for (i = start; i < end; i += step)
	hit[i]++;
      break;
    case 4:
      #pragma omp taskloop num_tasks (strict: arg)
      for (i = start; i < end; i += step)
	hit[i]++;
      break;
    case 5:
      #pragma omp taskloop if (0) grainsize (arg)
      for (i = start; i < end; i += step)
	hit[i]++;
      break;
    case 6:
      #pragma omp taskloop final (1) priority (5) num_tasks (arg)
      for (i = start; i < end; i += step)
	hit[i]++;
      break;
    case 7:
      #pragma omp taskgroup
      {
	#pragma omp taskloop nogroup num_tasks (arg)
	for (i = start; i < end; i += step)
	  hit[i]++;
      }
      break;
    }
}

__attribute__((noipa)) void
down (unsigned long long start, unsigned long long end,
      unsigned long long step, int kind, unsigned long long arg)
{
  unsigned long long i;

  switch (kind)
    {
    case 0:
      #pragma omp taskloop
      for (i = start; i > end; i -= step)
	hit[i]++;
      break;
    case 1:
      #pragma omp taskloop grainsize (arg)
      for (i = start; i > end; i -= step)
	hit[i]++;
      break;
    case 2:
      #pragma omp taskloop num_tasks (strict: arg)
      for (i = start; i > end; i -= step)
	hit[i]++;
      break;
    }
}

__attribute__((noipa)) void
vla (unsigned long long start, unsigned long long end, int n, int if_clause)
{
  int a[n];
  int j;
  unsigned long long i;

  for (j = 0; j < n; j++)
    a[j] = j;
  #pragma omp taskloop firstprivate (a) if (if_clause) num_tasks (3)
  for (i = start; i < end; i++)
    hit[i] += a[n - 1] == n - 1;
}

__attribute__((noipa)) unsigned long long
sum (unsigned long long start, unsigned long long end)
{
  unsigned long long i, s = 0;

  #pragma omp taskloop reduction (+: s) grainsize (3)
  for (i = start; i < end; i++)
    s += i;
  return s;
}

__attribute__((noipa)) void
in_final_task (unsigned long long start, unsigned long long end)
{
  #pragma omp task final (1)
  {
    unsigned long long i;

    #pragma omp taskloop num_tasks (5)
    for (i = start; i < end; i++)
      hit[i]++;
  }
  #pragma omp taskwait
}

__attribute__((noipa)) void
many_tasks (unsigned long long start, unsigned long long end)
{
  unsigned long long i;

  #pragma omp taskloop grainsize (1)
  for (i = start; i < end; i++)
    hit[i]++;
}

__attribute__((noipa)) void
with_children (unsigned long long start, unsigned long long end, int n)
{
  int a[n];
  unsigned long long i;

  a[n - 1] = 1;
  #pragma omp taskloop if (0) num_tasks (4)
  for (i = start; i < end; i++)
    {
      #pragma omp task firstprivate (i)
      child[i]++;
      hit[i]++;
    }
  #pragma omp taskloop if (0) num_tasks (4) firstprivate (a)
  for (i = start; i < end; i++)
    {
      #pragma omp task firstprivate (i)
      child[i] += a[n - 1];
      hit[i]++;
    }
}

int
main (void)
{
  int kind;
  unsigned long long arg;

  #pragma omp parallel
  #pragma omp single
  {
    for (kind = 0; kind < 8; kind++)
      for (arg = 1; arg < 8; arg++)
	{
	  up (3, 50, 1, kind, arg);
	  check (3, 50, 1);
	  up (5, 61, 4, kind, arg);
	  check (5, 61, 4);
	  up (10, 10, 1, kind, arg);
	  check (10, 10, 1);
	  up (2, 3, 5, kind, arg);
	  check (2, 3, 5);
	}
    up (0, N, 1, 1, 1000);
    check (0, N, 1);
    up (0, N, 1, 3, 1000);
    check (0, N, 1);
    for (kind = 0; kind < 3; kind++)
      for (arg = 1; arg < 8; arg++)
	{
	  down (60, 3, 1, kind, arg);
	  check_down (60, 3, 1);
	  down (63, 3, 5, kind, arg);
	  check_down (63, 3, 5);
	  down (7, 7, 1, kind, arg);
	  check_down (7, 7, 1);
	}
    vla (4, 40, 5, 1);
    check (4, 40, 1);
    vla (4, 40, 5, 0);
    check (4, 40, 1);
    if (sum (1, 101) != 5050ULL || sum (7, 7) != 0)
      abort ();
  }

  #pragma omp parallel num_threads (2)
  #pragma omp single
  {
    in_final_task (1, 50);
    check (1, 50, 1);
    many_tasks (0, N);
    check (0, N, 1);
    with_children (3, 40, 2);
  }

  /* The child tasks of an undeferred task lose their parent at its end, so
     only the barrier at the end of the parallel region waits for them.  */
  {
    unsigned long long i;

    for (i = 0; i < N; i++)
      if (hit[i] != 2 * (i >= 3 && i < 40)
	  || child[i] != 2 * (i >= 3 && i < 40))
	abort ();
  }
  return 0;
}
