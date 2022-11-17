// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

#include "test.h"

void *Thread(void *a) {
  barrier_wait(&barrier);
  *(int*)a = 43;
  return 0;
}

int main() {
  barrier_init(&barrier, 2);
  int Var __attribute__((annotate("dfi_protection"))) = 42;
  pthread_t t;
  pthread_create(&t, 0, Thread, &Var);
  Var = 43;
  barrier_wait(&barrier);
  pthread_join(t, 0);
}
