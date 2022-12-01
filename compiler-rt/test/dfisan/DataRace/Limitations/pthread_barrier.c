// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Dfisan gives a false alarm because static analysis cannot handle `pthread_barrier` correctly.

#include "test.h"

int shared __attribute__((annotate("dfi_protection")));
pthread_barrier_t bar;

void *Thread1(void *arg) {
  shared = 100;
  pthread_barrier_wait(&bar);
  return NULL;
}

void *Thread2(void *arg) {
  pthread_barrier_wait(&bar);
  shared;
  return NULL;
}

int main(void) {
  pthread_t tid[2];
  pthread_barrier_init(&bar, NULL, 2);

  pthread_create(&tid[0], NULL, Thread1, NULL);
  pthread_create(&tid[1], NULL, Thread2, NULL);
  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  pthread_barrier_destroy(&bar);

  return 0;
}
