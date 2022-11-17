// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include <stdlib.h>
#include "../DataRace/tsan/test.h"

struct Aligned {
  int x, y;
};

struct Unaligned {
  char c1, c2;
  int i;
};

void *Thread1(void *x) {
  barrier_wait(&barrier);
  int *p = (int *)x;
  *p = 500;
  return NULL;
}

void *Thread2(void *x) {
  int *p = (int *)x;
  *p;
  barrier_wait(&barrier);
  return NULL;
}

int main(int argc, char **argv) {
  struct Aligned aligned __attribute__((annotate("dfi_protection"))) = {100, 200};
  struct Unaligned unaligned __attribute__((annotate("dfi_protection"))) = {'a', 'b', 300};
  int noTarget = 400;
  int *p = (argc == 1) ? &aligned.x : (argc == 2) ? &unaligned.i : &noTarget;

  barrier_init(&barrier, 2);
  pthread_t t[2];
  barrier_init(&barrier, 2);
  pthread_create(&t[0], NULL, Thread1, p);
  pthread_create(&t[1], NULL, Thread2, p);
  pthread_join(t[0], NULL);
  pthread_join(t[1], NULL);

  return 0;
}
