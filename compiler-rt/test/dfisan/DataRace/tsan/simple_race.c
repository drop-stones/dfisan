// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

#include "test.h"

int Global __attribute__((annotate("dfi_protection")));

void *Thread1(void *x) {
  barrier_wait(&barrier);
  // Global = 42;
  Global;
  return NULL;
}

void *Thread2(void *x) {
  Global = 43;
  barrier_wait(&barrier);
  return NULL;
}

int main() {
  barrier_init(&barrier, 2);
  pthread_t t[2];
  pthread_create(&t[0], NULL, Thread1, NULL);
  pthread_create(&t[1], NULL, Thread2, NULL);
  pthread_join(t[0], NULL);
  pthread_join(t[1], NULL);
  return 0;
}

