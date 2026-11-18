/* { dg-do run } */

/* Test the loops with a runtime schedule for each schedule kind, with and
   without the monotonic and nonmonotonic modifiers, ordered and doacross,
   with long and unsigned long long iteration variables.  */

#include <omp.h>
#include <stdlib.h>

#define N 100

static int hit[N];
static int order[N];
static int next;

static void
check (int n)
{
  int i;

  for (i = 0; i < N; i++)
    {
      if (hit[i] != (i < n))
	abort ();
      hit[i] = 0;
    }
}

static void
check_order (int n)
{
  int i;

  if (next != n)
    abort ();
  for (i = 0; i < n; i++)
    if (order[i] != i)
      abort ();
  next = 0;
}

__attribute__((noipa)) void
loops (int n)
{
  long i;
  unsigned long long u;

  #pragma omp parallel for schedule (runtime)
  for (i = 0; i < n; i++)
    hit[i]++;
  check (n);

  #pragma omp parallel for schedule (monotonic: runtime)
  for (i = 0; i < n; i++)
    hit[i]++;
  check (n);

  #pragma omp parallel for schedule (nonmonotonic: runtime)
  for (u = 0; u < n; u++)
    hit[u]++;
  check (n);

  #pragma omp parallel for schedule (runtime)
  for (i = n - 1; i >= 0; i--)
    hit[i]++;
  check (n);

  #pragma omp parallel
  {
    #pragma omp for schedule (runtime)
    for (i = 0; i < n; i++)
      hit[i]++;

    #pragma omp single
    check (n);

    #pragma omp for schedule (monotonic: runtime)
    for (u = 0; u < n; u++)
      hit[u]++;

    #pragma omp single
    check (n);

    #pragma omp for schedule (nonmonotonic: runtime)
    for (u = 0; u < n; u++)
      hit[u]++;

    #pragma omp single
    check (n);

    #pragma omp for ordered schedule (runtime)
    for (i = 0; i < n; i++)
      {
	#pragma omp ordered
	order[next++] = i;
      }

    #pragma omp single
    check_order (n);

    #pragma omp for ordered schedule (runtime)
    for (u = 0; u < n; u++)
      {
	#pragma omp ordered
	order[next++] = u;
      }

    #pragma omp single
    check_order (n);

    #pragma omp for ordered (1) schedule (runtime)
    for (i = 0; i < n; i++)
      {
	#pragma omp ordered doacross (sink: i - 1)
	order[i] = i;
	#pragma omp ordered doacross (source:)
      }

    #pragma omp single
    {
      next = n;
      check_order (n);
    }

    /* The loop starts at 1, so that the sink iteration u - 1 is no wrapped
       value.  */
    #pragma omp for ordered (1) schedule (runtime)
    for (u = 1; u <= n; u++)
      {
	#pragma omp ordered doacross (sink: u - 1)
	order[u - 1] = u - 1;
	#pragma omp ordered doacross (source:)
      }

    #pragma omp single
    {
      next = n;
      check_order (n);
    }
  }
}

/* The task reduction makes the compiler call GOMP_loop_start and its
   ordered and doacross variants.  */

__attribute__((noipa)) void
task_reduction_loops (int n)
{
  long i, s = 0;
  unsigned long long u, t = 0;

  #pragma omp parallel
  {
    #pragma omp for schedule (runtime) reduction (task, +: s)
    for (i = 0; i < n; i++)
      s += i;

    #pragma omp for schedule (nonmonotonic: runtime) reduction (task, +: t)
    for (u = 0; u < n; u++)
      t += u;

    #pragma omp for ordered schedule (runtime) reduction (task, +: s)
    for (i = 0; i < n; i++)
      {
	#pragma omp ordered
	order[next++] = i;
	s += i;
      }

    #pragma omp single
    check_order (n);

    #pragma omp for ordered schedule (runtime) reduction (task, +: t)
    for (u = 0; u < n; u++)
      {
	#pragma omp ordered
	order[next++] = u;
	t += u;
      }

    #pragma omp single
    check_order (n);

    #pragma omp for ordered (1) schedule (runtime) reduction (task, +: s)
    for (i = 0; i < n; i++)
      {
	#pragma omp ordered doacross (sink: i - 1)
	s += i;
	#pragma omp ordered doacross (source:)
      }

    #pragma omp for ordered (1) schedule (runtime) reduction (task, +: t)
    for (u = 1; u <= n; u++)
      {
	#pragma omp ordered doacross (sink: u - 1)
	t += u - 1;
	#pragma omp ordered doacross (source:)
      }
  }
  if (s != 3L * n * (n - 1) / 2 || t != 3ULL * n * (n - 1) / 2)
    abort ();
}

/* The conditional lastprivate of an ordered loop and of a doacross loop
   with their default schedule.  */

__attribute__((noipa)) void
conditional_lastprivate_loops (int n)
{
  long i, c = -1, b = -1;
  unsigned long long u, d = 0, e = 0;

  #pragma omp parallel
  {
    #pragma omp for ordered lastprivate (conditional: d, e)
    for (u = 0; u < n; u++)
      {
	#pragma omp ordered
	order[next++] = u;
	if (u % 3 == 0)
	  d = u;
	if (u % 5 == 0)
	  e = u;
      }

    #pragma omp single
    check_order (n);

    #pragma omp for ordered (1) lastprivate (conditional: c, b)
    for (i = 0; i < n; i++)
      {
	#pragma omp ordered doacross (sink: i - 1)
	if (i % 4 == 0)
	  c = i;
	if (i % 6 == 0)
	  b = i;
	#pragma omp ordered doacross (source:)
      }
  }
  if (n > 0 && (d != (unsigned long long) (n - 1) / 3 * 3
		|| e != (unsigned long long) (n - 1) / 5 * 5
		|| c != (n - 1) / 4 * 4 || b != (n - 1) / 6 * 6))
    abort ();
}

int
main (void)
{
  static const struct
  {
    omp_sched_t kind;
    int chunk;
  } schedules[] = {
    { omp_sched_static, 0 },
    { omp_sched_static, 3 },
    { omp_sched_dynamic, 2 },
    { omp_sched_guided, 4 },
    { omp_sched_auto, 0 },
    { omp_sched_dynamic | omp_sched_monotonic, 5 },
    { omp_sched_dynamic, 40000 }
  };
  size_t s;

  for (s = 0; s < sizeof (schedules) / sizeof (schedules[0]); s++)
    {
      omp_set_schedule (schedules[s].kind, schedules[s].chunk);
      loops (N);
      loops (7);
      loops (0);
      task_reduction_loops (N);
      task_reduction_loops (0);
      conditional_lastprivate_loops (N);
      conditional_lastprivate_loops (0);
    }
  return 0;
}
