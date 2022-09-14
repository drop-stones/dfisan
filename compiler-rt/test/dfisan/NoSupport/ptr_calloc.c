// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests from 429.mcf in CPU2006.

#include <stdlib.h>

#define SIZE 144

struct arc;

struct node {
  struct arc *firstout;
};

struct arc {
  struct node *tail;
  struct arc *nextout;
};

struct network {
  struct node *nodes, *stop_node;
  struct arc *arcs, *stop_arc;
  int n;
};

void read_min(struct network *net) {
  struct arc *arc;
  struct node *node;

  net->n = SIZE;
  net->nodes = (struct node *)calloc(net->n + 1, sizeof(struct node));
  net->arcs = (struct arc *)calloc(net->n, sizeof(struct arc));

  node = net->nodes;
  arc = net->arcs;

  arc->tail = &(node[net->n]);
  arc->nextout = arc->tail->firstout;
}

int main(void) {
  struct network n;
  read_min(&n);

  return 0;
}
