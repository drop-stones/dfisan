// RUN: %clang_dfisan %s -o %t
// RUN: ! %run %t
//
// REQUIRES: x86_64-target-arch

#include "test.h"

int Global __attribute__((annotate("dfi_protection")));

void *Thread1(void *x) {
  barrier_wait(&barrier);
  Global = 42;
  return x;
}

int main() {
  barrier_init(&barrier, 2);
  pthread_t t;
  pthread_create(&t, 0, Thread1, 0);
  Global = 43;
  barrier_wait(&barrier);
  pthread_join(t, 0);
  return Global;
}
