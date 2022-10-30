// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan supports global pointer initializations.

#include <stdio.h>
#include <assert.h>

struct S {
  int *p;
  long *q;
};

int i       __attribute__((annotate("dfi_protection"))) = 100;
long l      __attribute__((annotate("dfi_protection"))) = 200;
int *r      __attribute__((annotate("dfi_protection"))) = &i;
long *s     __attribute__((annotate("dfi_protection"))) = &l;
struct S st __attribute__((annotate("dfi_protection"))) = { &i, &l };
int arr[8]  __attribute__((annotate("dfi_protection"))) = { 0, 1, 2, 3, 4, 5, 6, 7 };

int main(void) {
  printf("&i = %p, &l = %p, r = %p, s = %p, st.p = %p, st.q = %p\n",
         (void *)&i, (void *)&l, (void *)r, (void *)s, (void *)st.p, (void *)st.q);
  printf("i = %d, l = %ld, *r = %d, *s = %ld, *st.p = %d, *st.q = %ld\n",
         i, l, *r, *s, *st.p, *st.q);

  assert((i == 100 && l == 200) && "Int is not initialized");
  assert((r == &i && s == &l) && "Ptr is not initialized");
  assert((st.p == &i && st.q == &l) && "Struct is not initialized");
  for (int i = 0; i < 8; i++)
    assert(arr[i] == i && "Array is not initialized");

  return 0;
}
