#ifndef MMUTILS_H
#define MMUTILS_H

#include "mm.h"

#include <stdbool.h>
/* Making an actual comment */
/* Queue API */
int q_isempty();

void q_add(struct cell *p);

struct cell *q_peek();

void q_pop();

void q_reset();

bool q_is_processed(int nx, int ny);

void q_status();

/* Stack API */

int stack_isempty();

void stack_add(struct cell *p);

struct cell *stack_peek();

void stack_pop();

void stack_reset();

bool stack_is_processed(int nx, int ny);

void stack_status();

#endif /* MMUTILS_H */
