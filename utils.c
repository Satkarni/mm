#include "utils.h"
#include "mm.h"

#include <stdio.h>
#include <stdint.h>

// Container for cell pointer queue 
static struct cell *arr[MAZE_SIZE * MAZE_SIZE];

// Container for cell pointers stack
static struct cell *sarr[MAZE_SIZE * MAZE_SIZE];

struct cq {
  int r;
  int w;
  int count;
};

struct cstack{
  int r;
  int w;
  int count;
};

static struct cq cq = {.r = 0, .w = 0, .count = 0};
static struct cstack cstack = {.r = 0, .w = -1, .count = 0};

/* Queue API */

int q_isempty() {
  return (cq.count == 0);
}

void q_add(struct cell *p) {
  arr[cq.w] = p;
  cq.w = (cq.w + 1) % (MAZE_SIZE * MAZE_SIZE);
  cq.count++;
}

struct cell *q_peek() {
  if (!q_isempty())
    return arr[cq.r];
  else
    return NULL;
}

void q_pop() {
  if (!q_isempty()) {
    cq.r = (cq.r + 1) % (MAZE_SIZE * MAZE_SIZE);
    cq.count--;
  }
}

void q_reset() {
  cq.w = cq.r = cq.count = 0;
}

// TODO: Make more efficient later on w/ hashing
bool q_is_processed(int nx, int ny) {
  // If cell (nx, ny) is currently in the queue, skip
  // processing it again
  for (int i = 0; i < cq.count; i++) {
    struct cell *in_queue = arr[i];
    if (in_queue->x == nx && in_queue->y == ny) {
      return true;
    }
  }
  return false;
}

void q_status() {
  //mvprintw(41, 0, "c %d, r %d, w %d\n", cq.count, cq.r, cq.w);
}

/* Stack API */

int stack_isempty() {
  return (cstack.count == 0);
}

void stack_add(struct cell *p) {
  sarr[++cstack.w] = p;
  cstack.count++;
}

struct cell *stack_peek() {
  if (!stack_isempty())
    return sarr[cstack.w];
  else
    return NULL;
}

void stack_pop() {
  if (!stack_isempty()) {
    cstack.w--;
    cstack.count--;
  }
}


void stack_reset() {
   cstack.r = cstack.count = 0;
   cstack.w = -1;
}

bool stack_is_processed(int nx, int ny) {
    // If cell (nx, ny) is currently in the stack, skip
    // processing it again
    for (int i = 0; i < cstack.count; i++) {
        struct cell *in_stack= sarr[i];
        if(in_stack!= NULL){
            if (in_stack->x == nx && in_stack->y == ny) {
                return true;
            }
        }
        return false;
    }
}

void stack_status() {
  //mvprintw(41, 0, "c %d, r %d, w %d\n", cstack.count, cstack.r, cstack.w);
}
