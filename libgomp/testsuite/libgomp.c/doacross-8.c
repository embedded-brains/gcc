/* { dg-do run } */

/* Test sink iterations of unsigned iteration variables which wrap around.
   Such a sink iteration lies outside the iteration space, so the doacross
   dependence is ignored.  The sink i - 1, j orders the accesses to B
   where the other sink iteration lies outside the iteration space.  */

#include <limits.h>
#include <stdlib.h>

#define N 64

int a[N];
int b[N][8];

int
main (void)
{
  unsigned long long u;
  unsigned long long v;
  unsigned int i, j;

  /* An upward loop which starts at 0 with a negative offset.  */
  #pragma omp parallel for ordered (1) schedule (static, 1)
  for (u = 0; u < N; u++)
    {
      #pragma omp ordered doacross (sink: u - 1)
      a[u] = u ? a[u - 1] + 1 : 1;
      #pragma omp ordered doacross (source:)
    }
  for (i = 0; i < N; i++)
    if (a[i] != (int) i + 1)
      abort ();

  /* An upward loop which ends at UINT_MAX with a positive offset.  */
  #pragma omp parallel for ordered (2) schedule (static, 1)
  for (i = 0; i < N; i++)
    for (j = UINT_MAX - 6; j < UINT_MAX; j++)
      {
	unsigned int k = j - (UINT_MAX - 6);

	#pragma omp ordered doacross (sink: i - 1, j + 2) \
				    doacross (sink: i - 1, j)
	b[i][k] = i ? b[i - 1][k] + 1 : 1;
	#pragma omp ordered doacross (source:)
      }
  for (i = 0; i < N; i++)
    for (j = 0; j < 6; j++)
      if (b[i][j] != (int) i + 1)
	abort ();

  /* A downward loop which starts at ULLONG_MAX with a positive offset.  */
  #pragma omp parallel for ordered (1) schedule (static, 1)
  for (v = ULLONG_MAX; v > ULLONG_MAX - N; v--)
    {
      unsigned long long k = ULLONG_MAX - v;

      #pragma omp ordered doacross (sink: v + 1)
      a[k] = k ? a[k - 1] + 2 : 2;
      #pragma omp ordered doacross (source:)
    }
  for (i = 0; i < N; i++)
    if (a[i] != 2 * ((int) i + 1))
      abort ();

  /* A downward loop which ends at 0 with a negative offset.  */
  #pragma omp parallel for ordered (2) schedule (static, 1)
  for (i = 0; i < N; i++)
    for (j = 7; j > 0; j--)
      {
	#pragma omp ordered doacross (sink: i - 1, j - 2) \
				    doacross (sink: i - 1, j)
	b[i][j] = i ? b[i - 1][j] + 2 : 2;
	#pragma omp ordered doacross (source:)
      }
  for (i = 0; i < N; i++)
    for (j = 1; j < 8; j++)
      if (b[i][j] != 2 * ((int) i + 1))
	abort ();

  return 0;
}
