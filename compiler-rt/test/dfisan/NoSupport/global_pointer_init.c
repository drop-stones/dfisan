// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan supports global pointer initializations.
// TODO: @r = @i is not work. r = (nil) in this implementation

#include <stdio.h>

struct S {
  int *p;
  long *q;
};

///*
int i       __attribute__((annotate("dfi_protection"))) = 100;
long l      __attribute__((annotate("dfi_protection"))) = 200;
int *r      __attribute__((annotate("dfi_protection"))) = &i;
long *s     __attribute__((annotate("dfi_protection"))) = &l;
struct S st __attribute__((annotate("dfi_protection"))) = { &i, &l };
//*/
/*
int i       = 100;
long l      = 200;
int *r      = &i;
long *s     = &l;
struct S st = { &i, &l };
//*/


int main(void) {
/*
  printf("&i = %p, &l = %p, r = %p, s = %p, st.p = %p, st.q = %p\n",
         (void *)&i, (void *)&l, (void *)r, (void *)s, (void *)st.p, (void *)st.q);
  printf("i = %d, l = %ld, *r = %d, *s = %ld, *st.p = %d, *st.q = %ld\n",
         i, l, *r, *s, *st.p, *st.q);
*/
  printf("&i = %p, &l = %p, i = %d, l = %ld\n", (void *)&i, (void *)&l, i, l);

  return 0;
}
