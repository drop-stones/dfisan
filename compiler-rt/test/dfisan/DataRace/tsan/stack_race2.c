// RUN: %clang_dfisan %s -o %t
// RUN: ! %run %t
//
// REQUIRES: x86_64-target-arch

#include "test.h"

void *Thread2(void *a) {
  barrier_wait(&barrier);
  *(int*)a = 43;
  return 0;
}

void *Thread(void *a) {
  int Var __attribute__((annotate("dfi_protection"))) = 42;
  pthread_t t;
  pthread_create(&t, 0, Thread2, &Var);
  Var = 42;
  barrier_wait(&barrier);
  pthread_join(t, 0);
  return 0;
}

int main() {
  barrier_init(&barrier, 2);
  pthread_t t;
  pthread_create(&t, 0, Thread, 0);
  pthread_join(t, 0);
}
