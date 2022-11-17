// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

#include "test.h"

int GlobalData[10] __attribute__((annotate("dfi_protection")));

void *Thread(void *a) {
  barrier_wait(&barrier);
  GlobalData[2] = 42;
  return 0;
}

int main() {
  barrier_init(&barrier, 2);
  // print_address("addr=", 1, GlobalData);
  pthread_t t;
  pthread_create(&t, 0, Thread, 0);
  GlobalData[2] = 43;
  barrier_wait(&barrier);
  pthread_join(t, 0);
}
