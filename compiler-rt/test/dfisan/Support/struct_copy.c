// RUN: %clang_dfisan %s -o %t
// RUN: %run %t
//
// REQUIRES: x86_64-target-arch

// Tests that dfisan can manage struct copy.

#include <stddef.h>
#include <stdlib.h>

struct ListNode {
  struct ListNode *Next;
  int id;
  char name[8];
};

struct ListNode *createNode(int id) {
  struct ListNode *NewNode = (struct ListNode *)malloc(sizeof(struct ListNode));
  NewNode->id = id;
  for (int i = 0; i < 8; i++)
    NewNode->name[i] = 'a';
  return NewNode;
}

/*
struct ListNode *createList(int size) {
  struct ListNode *Head = NULL;
  for (int i = 0; i < size; i++) {
    struct ListNode *NewNode = createNode(i);
    NewNode->Next = Head;
    Head = NewNode;
  }
  return Head;
}
*/

void copyStruct() {
  struct ListNode N1 = { NULL, 100, "Alice" };
  struct ListNode N2 = { &N1, 200, "Bob" };
  struct ListNode tmp;

  N1.Next;
  N1.id;
  N1.name[0];
  N1.name[3];
  N2.Next;
  N2.id;
  N2.name[0];
  N2.name[3];

  // Swap N1 <-> N2
  tmp = N1;
  N1 = N2;
  N2 = tmp;

  N1.Next;
  N1.id;
  N1.name[0];
  N1.name[3];
  N2.Next;
  N2.id;
  N2.name[0];
  N2.name[3];
}

void copyStructUsingPtr() {
  struct ListNode *N3, *N4;
  N3 = createNode(300);
  N4 = createNode(400);

  N3->Next;
  N3->id;
  N3->name[0];
  N3->name[4];
  N4->Next;
  N4->id;
  N4->name[0];
  N4->name[4];

  // Swap N3 <-> N4
  struct ListNode tmp;
  tmp = *N3;
  *N3 = *N4;
  *N4 = tmp;

  N3->Next;
  N3->id;
  N3->name[0];
  N3->name[4];
  N4->Next;
  N4->id;
  N4->name[0];
  N4->name[4];
}

struct ListNode N5 = { NULL, 500, "Charlie" };
struct ListNode N6 = { &N5, 600, "Dave" };
struct ListNode gtmp;

void copyGlobalStruct() {
  N5.Next;
  N5.id;
  N5.name[0];
  N5.name[3];
  N6.Next;
  N6.id;
  N6.name[0];
  N6.name[3];

  // Swap N5 <-> N6
  gtmp = N5;
  N5 = N6;
  N6 = gtmp;

  N5.Next;
  N5.id;
  N5.name[0];
  N5.name[3];
  N6.Next;
  N6.id;
  N6.name[0];
  N6.name[3];
}

int main(void) {
  copyStruct();
  copyStructUsingPtr();
  copyGlobalStruct();

  return 0;
}