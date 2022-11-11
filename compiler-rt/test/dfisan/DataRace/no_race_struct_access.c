// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include <pthread.h>

#define CNT 100000

struct Vec2D {
  int x, y;
};

struct Vec2D v __attribute__((annotate("dfi_protection"))) = { 100, 200 };

void *access_x() {
  for (int i = 0; i < CNT; i++) {
    v.x;
    v.x = 100;
  }
  return NULL;
}

void *access_y() {
  for (int i = 0; i < CNT; i++) {
    v.y;
    v.y = 200;
  }
  return NULL;
}

int main(void) {
  pthread_t tid[2];

  pthread_create(&tid[0], NULL, access_x, NULL);
  pthread_create(&tid[1], NULL, access_y, NULL);
  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);

  return 0;
}
