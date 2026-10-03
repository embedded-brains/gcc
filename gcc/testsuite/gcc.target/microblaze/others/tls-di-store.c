/* { dg-do compile } */
/* { dg-require-effective-target tls_native } */
/* { dg-options "-O2" } */
/* { dg-skip-if "" { *-*-* } { "-flto" } { "" } } */

__thread unsigned long long x;

void
set (unsigned int v)
{
  x = v;
}

/* { dg-final { scan-assembler-not "swi\tr\[0-9\]+,r0,x" } } */
/* { dg-final { scan-assembler-times "x@TLSDTPREL" 2 } } */
