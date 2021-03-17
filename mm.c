// demo.c
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

// Maze file
#include "yama2002.c" // NOLINT(bugprone-suspicious-include)

// Micromouse struct and constants definitions
#include "mm.h"

#define DELAY_MILLIS  100000

int max_x, max_y;
int cell_width = 5, cell_height = 2;


static struct mm_pose mm_pose = {.x=0, .y=(MAZE_SIZE - 1), .curr_direction = _n};
static struct maze maze;

/* Get mouse directional symbol based on the direction */
char get_mouse_symbol(dir d) {
  char c = '-';
  switch (d) {
    case _n:
      c = '^';
      break;
    case _e:
      c = '>';
      break;
    case _w:
      c = '<';
      break;
    case _s:
      c = 'v';
      break;
    default:
      break;
  }
  return c;
}

int max(int a, int b) {
  return (a > b) ? a : b;
}

int min(int a, int b) {
  return (a < b) ? a : b;
}

/* Draw the rectangle for the current cell as the micromouse discovers it */
void draw_cell_rectangle(int offset, int x, int y, short wbm) {
  // Each cell will be  w by  h px
  int y1, x1, y2, x2;

  // 0,0
  //      1,1
  //          2,2
  y1 = cell_height * y + 1;//max_y/4;
  x1 = cell_width * x + offset;//max_x/4;
  y2 = y1 + cell_height;
  x2 = x1 + cell_width;


  // draw lines as per wall bitmap
  // n e w s ---> 3 2 1 0

  if (wbm & N) {   // north wall
    mvhline(y1, x1, 0, x2 - x1);
  }
  if (wbm & E) {   // east wall
    mvvline(y1, x2, 0, y2 - y1);
  }
  if (wbm & W) {   // west wall
    mvvline(y1, x1, 0, y2 - y1);
  }
  if (wbm & S) {   // south wall
    mvhline(y2, x1, 0, x2 - x1);
  }

  //          N
  //        _____
  //       |     |
  //      W|     |E
  //        _____
  //          S
  //
  if ((wbm & N) && (wbm & W)) mvaddch(y1, x1, ACS_ULCORNER);
  if ((wbm & S) && (wbm & W)) mvaddch(y2, x1, ACS_LLCORNER);
  if ((wbm & N) && (wbm & E)) mvaddch(y1, x2, ACS_URCORNER);
  if ((wbm & S) && (wbm & E)) mvaddch(y2, x2, ACS_LRCORNER);
}


void get_center(int x, int y, int *dx, int *dy) {
  *dx = /*max_x/4*/3 + cell_width * x + cell_width / 2;
  *dy = /*max_y/4*/1 + cell_height * y + cell_height / 2;
}

// abstraction to read sensor value and get actual wall data
// currently reads wall data from a sample maze, in actual case 
// will get wall data from sensors
short discover_walls(int x, int y) {
  short w = 0;

  // Translation between this code's x-y orientation and wall data from maz file
  // x,y (0,0)   => 15
  // x,y (0,1)   => 14
  // x,y (0,15)  => 0
  // x,y (15,0)  => 255
  // x,y (15,15) => 240
  w = yama2002_maz[MAZE_SIZE - 1 + MAZE_SIZE * x - y];
  return w;
}

// Get wall data from maz file
// maze will be initially empty, as mouse searches through the maze
// wall data will be updated
short get_walls(int x, int y) {
  short w = 0;

  // Translation between this code's x-y orientation and wall data from maz file
  // x,y (0,0)   => 15
  // x,y (0,1)   => 14
  // x,y (0,15)  => 0
  // x,y (15,0)  => 255
  // x,y (15,15) => 240
  w = cur_maze[MAZE_SIZE - 1 + MAZE_SIZE * x - y];
  return w;
}

void set_walls(int x, int y, short walls) {
  maze.cells[x][y].wbm = walls;
  cur_maze[MAZE_SIZE - 1 + MAZE_SIZE * x - y] = walls;
}

void draw_maze() {
  for (int i = 0; i < MAZE_SIZE; i++) {
    for (int j = 0; j < MAZE_SIZE; j++) {
      draw_cell_rectangle(3, i, j, get_walls(i, j));
      int x, y;
      get_center(i, j, &x, &y);
      mvprintw(y, x, "%d", maze.cells[i][j].value);
    }
  }
}

void draw_maze_actual() {
  for (int i = 0; i < MAZE_SIZE; i++) {
    for (int j = 0; j < MAZE_SIZE; j++) {
      draw_cell_rectangle(85, i, j, discover_walls(i, j));
      int x, y;
      get_center(i, j, &x, &y);
      //mvprintw(y,x,"%curr_direction",maze.cells[i][j].value);
    }
  }
}

// returns 1 if cell is valid
int check_coord_valid(int x, int y) {
  if (x < 0 || x >= MAZE_SIZE || y < 0 || y >= MAZE_SIZE)
    return 0;
  else
    return 1;
}

// check if the cell with nx and ny is open to cell c
// returns 1 if nbr is open
int check_if_nbr_open(struct cell *c, dir nbr_dir) {
  int walls = get_walls(c->x, c->y);
  if (nbr_dir == _n && !(walls & N)) {
    return 1;
  }
  if (nbr_dir == _e && !(walls & E)) {
    return 1;
  }
  if (nbr_dir == _w && !(walls & W)) {
    return 1;
  }
  if (nbr_dir == _s && !(walls & S)) {
    return 1;
  }

  return 0;
}

// returns pointer to valid and open nbr
struct cell *get_nbr(dir d, struct cell *p) {
  int isvalid = 0, isopen = 0;
  int nx, ny;
  if (d == _n) {
    nx = p->x;
    ny = p->y - 1;
  }
  if (d == _e) {
    nx = p->x + 1;
    ny = p->y;
  }
  if (d == _s) {
    nx = p->x;
    ny = p->y + 1;
  }
  if (d == _w) {
    nx = p->x - 1;
    ny = p->y;
  }
  isvalid = check_coord_valid(nx, ny);
  isopen = check_if_nbr_open(p, d);
  if (isvalid && isopen) {
    return &maze.cells[nx][ny];
  }
  return NULL;
}

// gets nbrs of c and fills list with them
// returns number of open and valid nbrs
int get_nbrs(struct cell **list, struct cell *c) {
  int i = 0;
  if (list == NULL || c == NULL) return -1;

  // find valid nbrs of c
  int nbrx, nbry;
  for (dir nbdir = _n; nbdir <= _s; nbdir++) {
    switch (nbdir) {
      case _n:
        nbrx = c->x;
        nbry = c->y - 1;
        break;
      case _e:
        nbrx = c->x + 1;
        nbry = c->y;
        break;
      case _w:
        nbrx = c->x - 1;
        nbry = c->y;
        break;
      case _s:
        nbrx = c->x;
        nbry = c->y + 1;
        break;
    }

    int isvalid = check_coord_valid(nbrx, nbry);
    int isopen = check_if_nbr_open(c, nbdir);

    if (isvalid && isopen) {
      // add to nbr list
      list[i++] = &maze.cells[nbrx][nbry];
    }
  }
  return i;
}

// sort nbr list based on value
void sort_nbrs(struct cell **list, int num) {
  for (int i = 0; i < num - 1; i++) {
    for (int j = 0; j < num - 1 - i; j++) {
      if ((list[j])->value > (list[j + 1])->value) {
        struct cell *tmp = list[j];
        list[j] = list[j + 1];
        list[j + 1] = tmp;
      }
    }
  }
}

// container for cell pointer queue
struct cell *arr[MAZE_SIZE * MAZE_SIZE];

struct cq {
  int r;
  int w;
  int count;
};

struct cq cq = {.r = 0, .w = 0, .count = 0};

int q_isempty() {
  return (cq.count == 0);
}

void add_q(struct cell *p) {
  arr[cq.w] = p;
  mvprintw(45, 0, "add x,y %d,%d", p->x, p->y);
  cq.w = (cq.w + 1) % (MAZE_SIZE * MAZE_SIZE);
  cq.count++;
}

struct cell *peek_q() {
  if (!q_isempty())
    return arr[cq.r];
  else
    return NULL;
}

void pop_q() {
  if (!q_isempty()) {
    cq.r = (cq.r + 1) % (MAZE_SIZE * MAZE_SIZE);
    cq.count--;
  }
}

void q_status() {
  mvprintw(41, 0, "c %d, r %d, w %d\n", cq.count, cq.r, cq.w);
}

void reset_q() {
  cq.w = cq.r = cq.count = 0;
}

void floodfill(int dx, int dy, int sx, int sy) {
  // add sx,sy to queue of cell pointers
  struct cell *p = &(maze.cells[sx][sy]);
  p->value = 0;
  add_q(p);

  int cx = sx, cy = sy;

  while (!(cx == dx && cy == dy)) {

    // get one cell from q to analyze its nbrs
    struct cell *tmp = peek_q();
    if (!tmp) {
      mvprintw(43, 0, "cx %d,cy %d, Aborting...", cx, cy);
      sleep(10);
      break;
    } else {
      pop_q();    // delete from q if peek successful
    }
    cx = tmp->x;
    cy = tmp->y;
    //tmp->visited = 1;

    // get valid open nbrs
    struct cell *nbr_n = get_nbr(_n, tmp);
    struct cell *nbr_e = get_nbr(_e, tmp);
    struct cell *nbr_s = get_nbr(_s, tmp);
    struct cell *nbr_w = get_nbr(_w, tmp);

    int newval = tmp->value + 1;
    if (nbr_n != NULL && nbr_n->value > newval) {
      nbr_n->value = newval;
      add_q(nbr_n);
    }
    if (nbr_e != NULL && nbr_e->value > newval) {
      nbr_e->value = newval;
      add_q(nbr_e);
    }
    if (nbr_s != NULL && nbr_s->value > newval) {
      nbr_s->value = newval;
      add_q(nbr_s);
    }
    if (nbr_w != NULL && nbr_w->value > newval) {
      nbr_w->value = newval;
      add_q(nbr_w);
    }
  }

  reset_q();
}

int put_in_bounds(int val, int min_val, int max_val) {
  val = max(val, min_val);
  val = min(val, max_val);
  return val;
}

bool is_move_legal(dir direction, int x, int y) {
  int is_wall_present = 1;

  switch (direction) {
    case _n:
      is_wall_present = maze.cells[x][y].wbm & N;
      break;
    case _e:
      is_wall_present = maze.cells[x][y].wbm & E;
      //mvprintw(40, 0, "is_wall_present: %d", maze.cells[x][y].wbm);
      break;
    case _s:
      is_wall_present = maze.cells[x][y].wbm & S;
      break;
    case _w:
      is_wall_present = maze.cells[x][y].wbm & W;
      break;
  }
  return (is_wall_present == 0) ? true : false;
}

void make_pose_update(const dir direction) {
  int new_x = mm_pose.x;
  int new_y = mm_pose.y;

  if (is_move_legal(direction, mm_pose.x, mm_pose.y)) {
    switch (direction) {
      case _n:
        new_y = put_in_bounds(new_y - 1, 0, MAZE_SIZE - 1);
        break;
      case _e:
        new_x = put_in_bounds(new_x + 1, 0, MAZE_SIZE - 1);
        break;
      case _w:
        new_x = put_in_bounds(new_x - 1, 0, MAZE_SIZE - 1);
        break;
      // _s
      default:
        new_y = put_in_bounds(new_y + 1, 0, MAZE_SIZE - 1);
        break;
    }
    mm_pose.x = new_x;
    mm_pose.y = new_y;
    mm_pose.curr_direction = direction;
  }
}

void get_direction_input(const int c) {
  switch (c) {
    case KEY_UP:
      make_pose_update(_n);
      break;
    case KEY_RIGHT:
      make_pose_update(_e);
      break;
    case KEY_LEFT:
      make_pose_update(_w);
      break;
    case KEY_DOWN:
      make_pose_update(_s);
      break;
    default:
      break;
  }
}

int main(int argc, char *argv[]) {

  initscr();
  cbreak();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);
  scrollok(stdscr, TRUE);
  noecho();
  getmaxyx(stdscr, max_y, max_x);
  printf("%d, %d", max_x, max_y);
  curs_set(FALSE);

  srand(time(NULL));

  // Initialization of maze
  for (int i = 0; i < MAZE_SIZE; i++) {
    for (int j = 0; j < MAZE_SIZE; j++) {
      maze.cells[i][j].x = i;
      maze.cells[i][j].y = j;
      maze.cells[i][j].value = 255;
    }
  }
  static char s;

  int f_flood = 0;
  clear();

  draw_maze();
  draw_maze_actual();

  while (1) {
    clear();
    int f_flood = 1;

    short newwall = discover_walls(mm_pose.x, mm_pose.y);
    set_walls(mm_pose.x, mm_pose.y, newwall);

    int c = getch();
    // Get manual user movement in maze
    get_direction_input(c);

//        if(f_flood) {
//               for(int i =0 ;i<SZ;i++){
//                    for(int j=0;j<SZ;j++){
//                        maze.cells[i][j].value = 255;
//                    }
//                }
//            floodfill(mm.x,mm.y,8,8);
//            f_flood = 0;
//        }


    draw_maze();
    draw_maze_actual();
    get_center(mm_pose.x, mm_pose.y, &mm_pose.mx, &mm_pose.my);
    s = get_mouse_symbol(mm_pose.curr_direction);
    mvprintw(mm_pose.my, mm_pose.mx, &s);

    //mvprintw(40,0,"Status: ");
    //mvprintw(40,10,"x: %d, y: %d",mm.x,mm.y);
    //q_status();

    refresh();
    usleep(DELAY_MILLIS);
  }

  endwin(); // Restore normal terminal behavior

}
