// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

#include "test.h"
#include "../../safe_alloc.h"
#include <pthread.h>
#include <stdio.h>
#include <stddef.h>

void *Thread(void *a) {
  ((int*)a)[0]++;
  barrier_wait(&barrier);
  return NULL;
}

int main() {
  barrier_init(&barrier, 2);
  int *p = (int *)safe_malloc(sizeof(int));
  *p = 42;
  pthread_t t;
  pthread_create(&t, NULL, Thread, p);
  barrier_wait(&barrier);
  p[0]++;
  pthread_join(t, NULL);
  free(p);
}
