/*
 * Simple linked list-based queue example
 *
 * Based on list.c. Violates POM because tail is mutable
 *
 * Yow! You can't create a queue out of a singly-linked list! (because
 * must point to tail)
 * https://rust-unofficial.github.io/too-many-lists/fifth-layout.html
 * (assuming no Unsafe, no Cells (or interior mutability, and don't want
 * to traverse list to do push or pop) Does this mean you can't build a
 * big tree of pointers and grow the tree at the leaves?  To solve the
 * tail problem, you could use a RefCell, or raw pointer.  Or use Rc
 * pointers throughout the list.
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

typedef struct queue {
  Node* head;
  Node* tail;
} Queue;


Queue new_queue(void) {
  Queue queue;
  queue.head = NULL;
  queue.tail = NULL;
  return queue;
}

/* Pushes to tail */
void push(Queue *queue, char *new_data) {
  Node *new_node = malloc(sizeof(Node));
  if (new_node == NULL) {
    abort();
  }
  new_node->data = new_data;
  new_node->next = NULL;
  if (queue->tail == NULL) {
    queue->head = new_node;
    queue->tail = new_node;
  } else {
    queue->tail->next = new_node;
    queue->tail = queue->tail->next;
  }
}

/* Pops from head */
char *pop(Queue *queue) {
  if (queue->head == NULL) {
    return NULL;
  }
  Node *popped = queue->head;
  char *result = popped->data;
  Node *new_head = popped->next;
  queue->head = new_head;
  if (new_head == NULL) {
    queue->tail = NULL;
  }
  free(popped);
  return result;
}

char *peek(Queue queue) {
  if (queue.head == NULL) {
    return NULL;
  }
  return queue.head->data;
}

void erase(Queue *queue) {
  Node *node = queue->head;
  while (node != NULL) {
    Node *next = node;
    free(node);
    node = next;
  }
}

int main(void) {
  Queue queue = new_queue();
  assert(pop(&queue) == NULL);

  push(&queue, "Alice");
  push(&queue, "Bob");
  push(&queue, "Carol");

  assert(!strcmp(pop(&queue), "Alice"));
  assert(!strcmp(pop(&queue), "Bob"));

  // Push some more just to make sure nothing's corrupted
  push(&queue, "David");
  push(&queue, "Ellen");

  // Check normal removal
  assert(!strcmp(pop(&queue), "Carol"));
  assert(!strcmp(pop(&queue), "David"));

  // Check exhaustion
  assert(!strcmp(pop(&queue), "Ellen"));
  assert(pop(&queue) == NULL);

  assert(peek(queue) == NULL);
  push(&queue, "Alice");
  push(&queue, "Bob");
  push(&queue, "Carol");

  assert(!strcmp(peek(queue), "Alice"));
  return 0;
}
