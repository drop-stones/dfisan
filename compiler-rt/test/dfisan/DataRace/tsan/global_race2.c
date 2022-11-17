// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

#include "test.h"

int x __attribute__((annotate("dfi_protection")));

void *Thread(void *a) {
  barrier_wait(&barrier);
  x = 1;
  return 0;
}

int main() {
  barrier_init(&barrier, 2);
  // print_address("addr2=", 1, &x);
  pthread_t t;
  pthread_create(&t, 0, Thread, 0);
  x = 0;
  barrier_wait(&barrier);
  pthread_join(t, 0);
}
