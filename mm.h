//
// Created by Nishant Kelkar on 3/14/21.
//
#ifndef MM_MM_H
#define MM_MM_H

#include <stdbool.h>

#define MAZE_SIZE     16
#define N  (1<<0)
#define E  (1<<1)
#define S  (1<<2)
#define W  (1<<3)

/* Orientation that the mouse is pointing in (N, E, W, S) */
typedef enum {
  _n,
  _e,
  _w,
  _s
} dir;

struct mm_pose {
  int x;
  int y;
  int mx;
  int my;
  dir curr_direction;
};

/**
 * Cell details, including the wall bit-map info. For example:
 * North => wbm = 0b0001
 * East => wbm = 0b0010
 * South => wbm = 0b0100
 * West => wbm = 0b1000
 * North and East => wbm = 0b0011
 * ...
 * If wbm = 0b0001 for a particular cell instance, that means
 * that there is a wall to the North of this cell.
 */
struct cell {
  int x;
  int y;
  int wbm;    // Wall bit-map
  bool visited; // Used in flood fill
  int phy_visited; // Has mouse physically visited this cell ?
  int value;
};

// Goal cells (center cells): (7,7) (7,8) (8,7) (8,8)
// Start cell: (0,15)
struct maze {
  struct cell cells[MAZE_SIZE][MAZE_SIZE];
};

#endif //MM_MM_H
