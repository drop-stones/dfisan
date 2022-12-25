// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include <stdio.h>
#include "test.h"
#include "../safe_alloc.h"

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
  pthread_t tid[2];

  pthread_create(&tid[0], NULL, SlaveStart, Global);
  pthread_create(&tid[1], NULL, SlaveStart, Global);

  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  Global->current_id;

  return 0;
}
