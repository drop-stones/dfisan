// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include <pthread.h>
#include "tsan/test.h"

char cond = 0;
int Global __attribute__((annotate("dfi_protection")));

void *write_if() {
  barrier_wait(&barrier);
  if (cond)
    Global = 42;
  return NULL;
}

void *read_if_not() {
  if (!cond)
    Global;
  barrier_wait(&barrier);
  return NULL;
}

int main() {
  barrier_init(&barrier, 2);
  pthread_t t[2];
  pthread_create(&t[0], NULL, write_if, NULL);
  pthread_create(&t[1], NULL, read_if_not, NULL);
  pthread_join(t[0], NULL);
  pthread_join(t[1], NULL);
  return 0;
}
