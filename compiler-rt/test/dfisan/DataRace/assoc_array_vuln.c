// RUN: %clang_dfisan %s -o %t
// RUN: !%run %t
//
// REQUIRES: x86_64-target-arch

// Data-race in linux/assoc_array.c found by OWL[DNS'18] (Figure 5)

#include "test.h"
#include "../safe_alloc.h"

struct assoc_array_ptr;

struct assoc_array_edit {
  struct {
    struct assoc_array_ptr **ptr;
    struct assoc_array_ptr *to;
  } set[2];
};


void add_key(struct assoc_array_edit *edit) {
  // ...
  edit->set[0].ptr = NULL;
  barrier_wait(&barrier);
}

void request_key(struct assoc_array_edit *edit) {
  // ...
  barrier_wait(&barrier);
  *edit->set[0].ptr;
}

void *Thread1(void *arg) {
  struct assoc_array_edit *edit = (struct assoc_array_edit *)arg;
  add_key(edit);
  return edit;
}

int main(void) {
  barrier_init(&barrier, 2);
  pthread_t tid;

  struct assoc_array_edit *edit = (struct assoc_array_edit *)safe_malloc(sizeof(struct assoc_array_edit));
  struct assoc_array_ptr *p, *q;
  edit->set[0].ptr = &p;
  edit->set[1].ptr = &q;

  pthread_create(&tid, NULL, Thread1, (void *)edit);

  request_key(edit);

  pthread_join(tid, NULL);

  return 0;
}
