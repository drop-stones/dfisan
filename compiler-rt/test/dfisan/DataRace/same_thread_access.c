// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

#include <stdio.h>
#include "test.h"
#include "../safe_alloc.h"

#define SIZE 8

struct Vec2D {
  int x, y;
};

void *write_twice(void *arg) {
  struct Vec2D *vec = (struct Vec2D *)arg;
  for (int i = 0; i < SIZE; i++) {
    vec->x += 100;
    vec->y += 200;
  }

  return NULL;
}

int main(void) {
  struct Vec2D *v = (struct Vec2D *)safe_calloc(sizeof(struct Vec2D), SIZE);
  pthread_t tid[SIZE];

  for (int i = 0; i < SIZE; i++)
    pthread_create(&tid[i], NULL, write_twice, &v[i]);
  for (int i = 0; i < SIZE; i++)
    pthread_join(tid[i], NULL);
  
  return 0;
}
