// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include "test.h"

void *SlaveStart(void *arg) {
  int *shared = (int *)arg;
  *shared += 100;
  return NULL;
}

int main(void) {
  int shared __attribute__((annotate("dfi_protection"))) = 0;
  pthread_t tid[2];
  pthread_create(&tid[0], NULL, SlaveStart, &shared);
  pthread_create(&tid[1], NULL, SlaveStart, &shared);
  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);
  shared;
  return 0;
}
