/*
 * Simple linked list example
 *
 * Rough C translation of Rust linked list from:
 * https://rust-unofficial.github.io/too-many-lists/second-final.html
 *
// <legal>
// Pointer Ownership Model (POM) Source Code Release
// 
// Copyright 2025 Carnegie Mellon University.
// 
// NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
// INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
// UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
// IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
// FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
// OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT
// MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
// TRADEMARK, OR COPYRIGHT INFRINGEMENT.
// 
// Licensed under a MIT (SEI)-style license, please see license.txt or
// contact permission@sei.cmu.edu for full terms.
// 
// [DISTRIBUTION STATEMENT A] This material has been approved for public
// release and unlimited distribution.  Please see Copyright notice for
// non-US Government use and distribution.
// 
// DM25-1262
// </legal>
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct node_s {
  char *data;
  struct node_s* next;
} Node;

typedef struct list {
  Node* head;
} List;


List new_list(void) {
  List list;
  list.head = NULL;
  return list;
}

void push(List *list, char *new_data) {
  Node *new_node = malloc(sizeof(Node));
  if (new_node == NULL) {
    abort();
  }
  new_node->data = new_data;
  new_node->next = list->head;
  list->head = new_node;
}

char *pop(List *list) {
  if (list->head == NULL) {
    return NULL;
  }
  Node *popped = list->head;
  char *result = popped->data;
  Node *new_head = popped->next;
  list->head = new_head;
  free(popped);
  return result;
}

char *peek(List list) {
  if (list.head == NULL) {
    return NULL;
  }
  return list.head->data;
}

void erase(List *list) {
  Node *node = list->head;
  list->head = NULL;
  while (node != NULL) {
    Node *next = node;
    free(node);
    node = next;
  }
  // list->head = NULL;  violates POM if this statement is down here rather than
  // before while loop, because list->head is a zombie throughout while loop
}

int main(void) {
  List list = new_list();
  assert(pop(&list) == NULL);

  push(&list, "Alice");
  push(&list, "Bob");
  push(&list, "Carol");

  assert(!strcmp(pop(&list), "Carol"));
  assert(!strcmp(pop(&list), "Bob"));

  // Push some more just to make sure nothing's corrupted
  push(&list, "David");
  push(&list, "Ellen");

  // Check normal removal
  assert(!strcmp(pop(&list), "Ellen"));
  assert(!strcmp(pop(&list), "David"));

  // Check exhaustion
  assert(!strcmp(pop(&list), "Alice"));
  assert(pop(&list) == NULL);

  assert(peek(list) == NULL);
  push(&list, "Alice");
  push(&list, "Bob");
  push(&list, "Carol");

  assert(!strcmp(peek(list), "Carol"));
  erase(&list);
  return 0;
}
