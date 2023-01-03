// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// dfisan report a false alarm because SVF cannot handle join sites of multi-forked threads.
// -> Update MHP.cpp to support multi-forked threads.

#include "test.h"
#include "../safe_alloc.h"

pthread_mutex_t m;
int shared __attribute__((annotate("dfi_protection")));

void *SlaveStart(void *arg) {
  pthread_mutex_lock(&m);
  shared++;
  pthread_mutex_unlock(&m);
  return NULL;
}

int main(void) {
  pthread_mutex_init(&m, NULL);
  shared = 0;
  unsigned nproc = 4;
  pthread_t tid[nproc];

  for (unsigned i = 0; i < nproc; i++)
    pthread_create(&tid[i], NULL, SlaveStart, NULL);
  
  for (unsigned i = 0; i < nproc; i++)
    pthread_join(tid[i], NULL);
  
  shared; // False positve

  return 0;
}
