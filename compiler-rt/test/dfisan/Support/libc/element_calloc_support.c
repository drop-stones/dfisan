// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage calloc() of struct elements.

#include <stdlib.h>

struct arc {
  int ident;
};

struct network_t {
  struct arc *arcs, *stop_arcs;
  int size;
};

struct network_t net;

// Write net
void read_min(struct network_t *net) {
  net->size = 100;
  net->arcs = (struct arc *)calloc(net->size, sizeof(struct arc));
  net->stop_arcs = net->arcs + net->size;
}

// Read net
void primal_start_artificial(struct network_t *net) {
  struct arc *stop = net->stop_arcs;
  for (struct arc *iter = net->arcs; iter != stop; iter++) {
    iter->ident;
  }
}

int main(void) {
  // memset( (void *)(&net), 0, sizeof(struct network_t));
  read_min(&net);
  primal_start_artificial(&net);

  return 0;
}