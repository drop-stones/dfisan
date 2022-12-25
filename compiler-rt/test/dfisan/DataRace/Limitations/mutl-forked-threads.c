// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// dfisan report a false alarm because SVF cannot handle join sites of multi-forked threads.

#include "test.h"
#include "../../safe_alloc.h"

struct GlobalMemory {
  pthread_mutex_t CountLock;
  long current_id;
};

void *SlaveStart(void *arg) {
  struct GlobalMemory *Global = (struct GlobalMemory *)arg;
  pthread_mutex_lock(&Global->CountLock);
  Global->current_id++;
  pthread_mutex_unlock(&Global->CountLock);
  return NULL;
}

int main(void) {
  struct GlobalMemory *Global = (struct GlobalMemory *)safe_malloc(sizeof(struct GlobalMemory));
  pthread_mutex_init(&Global->CountLock, NULL);
  Global->current_id = 0;
  unsigned nproc = 4;
  pthread_t tid[nproc];

  for (unsigned i = 0; i < nproc; i++)
    pthread_create(&tid[i], NULL, SlaveStart, Global);
  
  for (unsigned i = 0; i < nproc; i++)
    pthread_join(tid[i], NULL);
  
  Global->current_id;

  return 0;
}
