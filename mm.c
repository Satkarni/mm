// demo.c
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Maze file
#include "yama2002.c" // NOLINT(bugprone-suspicious-include)

// Micromouse struct and constants definitions
#include "mm.h"

#define DELAY_MILLIS  100000
#define KEY_ESC       27

int max_x, max_y;
int cell_width = 5, cell_height = 2;


static struct mm_pose mm_pose = {.x= 0, .y=(MAZE_SIZE - 1), .curr_direction = _n};
static struct maze maze;
FILE *fp;
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
  w = maze.cells[x][y].wbm;
  return w;
}

// Returns 1 if cell is valid
int check_coord_valid(int x, int y) {
  if (x < 0 || x >= MAZE_SIZE || y < 0 || y >= MAZE_SIZE)
    return 0;
  else
    return 1;
}

void set_walls(int x, int y, short walls) {
  maze.cells[x][y].wbm = walls;

  // Check for symmetrical wall updates to neighboring cells!
  // For example: updating my East wall info also means updating
  // the neighbor to my left's West wall info
  if ((walls & N) && check_coord_valid(x, y - 1)) {
    maze.cells[x][y - 1].wbm = (maze.cells[x][y - 1].wbm | S);
  }
  if ((walls & E) && check_coord_valid(x + 1, y)) {
    maze.cells[x + 1][y].wbm = (maze.cells[x + 1][y].wbm | W);
  }
  if ((walls & S) && check_coord_valid(x, y + 1)) {
    maze.cells[x][y + 1].wbm = (maze.cells[x][y + 1].wbm | N);
  }
  if ((walls & W) && check_coord_valid(x - 1, y)) {
    maze.cells[x - 1][y].wbm = (maze.cells[x - 1][y].wbm | E);
  }
}

void draw_maze() {
  for (int i = 0; i < MAZE_SIZE; i++) {
    for (int j = 0; j < MAZE_SIZE; j++) {
      draw_cell_rectangle(3, i, j, get_walls(i, j));
      int x, y;
      get_center(i, j, &x, &y);
      mvprintw(y, x, "%d" , maze.cells[i][j].value);
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

// Check if the cell with nx and ny is open to cell c
// Returns 1 if nbr is open
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

// Container for cell pointer queue
struct cell *arr[MAZE_SIZE * MAZE_SIZE];

struct cq {
  int r;
  int w;
  int count;
};

struct cq cq = {.r = 0, .w = 0, .count = 0};

// stack Container for cell pointer queue
struct cell *sarr[MAZE_SIZE * MAZE_SIZE];

struct cstack{
  int r;
  int w;
  int count;
};

struct cstack cstack = {.r = 0, .w = -1, .count = 0};

bool is_processed_stack(int nx, int ny) {
    // If cell (nx, ny) is currently in the stack, skip
    // processing it again
    for (int i = 0; i < cstack.count; i++) {
        struct cell *in_queue = sarr[i];
        if(in_queue != NULL){
            if (in_queue->x == nx && in_queue->y == ny) {
                return true;
            }
        }
        return false;
    }
}

// TODO: Make more efficient later on w/ hashing
bool is_processed(int nx, int ny) {
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

// Returns pointer to valid and open nbr
struct cell *get_nbr(dir d, struct cell *p) {
  int isvalid = 0, is_open = 0;
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
  is_open = check_if_nbr_open(p, d);
  if (isvalid && is_open) {
    return &maze.cells[nx][ny];
  }
  return NULL;
}

// Gets nbrs of c and fills list with them
// Returns number of open and valid nbrs
int get_nbrs(struct cell *list[], struct cell *c) {
  int i = 0;
  if (list == NULL || c == NULL) return -1;

  // Find valid nbrs of c
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
      // Add to nbr list
      list[i++] = &maze.cells[nbrx][nbry];
    }
  }
  return i;
}

// Sort nbr list based on value
void sort_nbrs(struct cell *list[], int num) {
  for (int i = 0; i < num - 1; i++) {
    for (int j = 0; j < num - 1 - i; j++) {
      if ((list[j])->value > (list[j + 1])->value
        || (list[j]->value == list[j+1]->value && list[j]->phy_visited == 1 && list[j+1]->phy_visited == 0) 
                                                    ) {
        struct cell *tmp = list[j];
        list[j] = list[j + 1];
        list[j + 1] = tmp;
      }
    }
  }
}

int q_isempty() {
  return (cq.count == 0);
}

void add_q(struct cell *p) {
  arr[cq.w] = p;
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

int stack_isempty() {
  return (cstack.count == 0);
}

void add_stack(struct cell *p) {
  sarr[++cstack.w] = p;
  cstack.count++;
}

struct cell *peek_stack() {
  if (!stack_isempty())
    return sarr[cstack.w];
  else
    return NULL;
}

void pop_stack() {
  if (!stack_isempty()) {
    cstack.w--;
    cstack.count--;
  }
}

void stack_status() {
  mvprintw(41, 0, "c %d, r %d, w %d\n", cstack.count, cstack.r, cstack.w);
}

void reset_stack() {
   cstack.r = cstack.count = 0;
   cstack.w = -1;
}

void dfs(int dst_x, int dst_y, int src_x, int src_y) {
    // First check validity of input args
    if (!check_coord_valid(dst_x, dst_y) || !check_coord_valid(src_x, src_y)) {
        return;
    }

    // Initialize queue w/ src cell
    reset_stack();

    // Add starting cell to queue
    struct cell *p = &(maze.cells[src_x][src_y]);
    p->value = 0;
    add_stack(p);
    int curr_sz;
    //fprintf(fp,"%s\n","stack start*****************************************");
    while (!stack_isempty()) {
        curr_sz = cstack.count;
        //fprintf(fp,"stack count %d\n",curr_sz);
        if(curr_sz == 0) break;

        for (int i = 0; i < curr_sz; i++) {
            // Dequeue a cell
            struct cell* curr = peek_stack();
            pop_stack();

            // Visit curr and mark its distance
            curr->visited = true;
            //fprintf(fp, "\ncurr %d,%d: ", curr->x, curr->y); 

            // Get all possible neighbors
            struct cell *nbr_n = get_nbr(_n, curr);
            struct cell *nbr_e = get_nbr(_e, curr);
            struct cell *nbr_s = get_nbr(_s, curr);
            struct cell *nbr_w = get_nbr(_w, curr);

            if (nbr_n != NULL && !nbr_n->visited && !is_processed_stack(nbr_n->x, nbr_n->y)) {
                nbr_n->value = curr->value + 1;
                add_stack(nbr_n);
                //fprintf(fp, "add %d,%d ", nbr_n->x, nbr_n->y);
            }
            if (nbr_e != NULL && !nbr_e->visited && !is_processed_stack(nbr_e->x, nbr_e->y)) {
                nbr_e->value = curr->value + 1;
                add_stack(nbr_e);
                //fprintf(fp, "add %d,%d ", nbr_e->x, nbr_e->y);
            }
            if (nbr_s != NULL && !nbr_s->visited && !is_processed_stack(nbr_s->x, nbr_s->y)) {
                nbr_s->value = curr->value + 1;
                add_stack(nbr_s);
                //fprintf(fp, "add %d,%d ", nbr_s->x, nbr_s->y);
            }
            if (nbr_w != NULL && !nbr_w->visited && !is_processed_stack(nbr_w->x, nbr_w->y)) {
                nbr_w->value = curr->value + 1;
                add_stack(nbr_w);
                //fprintf(fp, "add %d,%d ", nbr_w->x, nbr_w->y);
            }
            fflush(fp);
        }
  }
  //fprintf(fp,"\n%s\n","stack end**************************************"); 
  // Clean up after DFS is done
  reset_stack();
}

void bfs(int dst_x, int dst_y, int src_x, int src_y) {
  // First check validity of input args
  if (!check_coord_valid(dst_x, dst_y) || !check_coord_valid(src_x, src_y)) {
    return;
  }

  // Initialize queue w/ src cell
  reset_q();

  // Add starting cell to queue
  struct cell *p = &(maze.cells[src_x][src_y]);
  p->value = 0;
  add_q(p);

  int curr_sz;
  int dist = 0;
  //fprintf(fp,"%s\n","q start*****************************************");
  while (!q_isempty()) {
    curr_sz = cq.count;
    //fprintf(fp,"q count %d\n",curr_sz);
    if(curr_sz == 0) break;
    for (int i = 0; i < curr_sz; i++) {
      // Dequeue a cell
      struct cell* curr = peek_q();
      pop_q();

      // Visit curr and mark its distance
      curr->visited = true;
      curr->value = dist;
      //fprintf(fp, "\ncurr %d,%d: ", curr->x, curr->y); 
      // Get all possible neighbors
      struct cell *nbr_n = get_nbr(_n, curr);
      struct cell *nbr_e = get_nbr(_e, curr);
      struct cell *nbr_s = get_nbr(_s, curr);
      struct cell *nbr_w = get_nbr(_w, curr);

      if (nbr_n != NULL && !nbr_n->visited && !is_processed(nbr_n->x, nbr_n->y)) {
        add_q(nbr_n);
        //fprintf(fp, "add %d,%d ", nbr_n->x, nbr_n->y);
      }
      if (nbr_e != NULL && !nbr_e->visited && !is_processed(nbr_e->x, nbr_e->y)) {
        add_q(nbr_e);
        //fprintf(fp, "add %d,%d ", nbr_e->x, nbr_e->y);
      }
      if (nbr_s != NULL && !nbr_s->visited && !is_processed(nbr_s->x, nbr_s->y)) {
        add_q(nbr_s);
        //fprintf(fp, "add %d,%d ", nbr_s->x, nbr_s->y);
      }
      if (nbr_w != NULL && !nbr_w->visited && !is_processed(nbr_w->x, nbr_w->y)) {
        add_q(nbr_w);
        //fprintf(fp, "add %d,%d ", nbr_w->x, nbr_w->y);
      }
      fflush(fp);
    }
    dist += 1;
  }
  //fprintf(fp,"\n%s\n","q end**************************************"); 
  // Clean up after BFS is done
  reset_q();
}

// Sort nbr list based on value
void dijkstra_sort_nbrs(struct cell *list[], int num) {
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

void dijkstra(int dst_x, int dst_y, int src_x, int src_y) 
{
    // First check validity of input args
    if (!check_coord_valid(dst_x, dst_y) || !check_coord_valid(src_x, src_y)) {
        return;
    }

    reset_stack();

    struct cell *c = &maze.cells[src_x][src_y];
    c->value = 0;

    // Add it to path
    add_q(c);

    while(!q_isempty()){
        fprintf(fp,"count: %d\n", cq.count);
        struct cell *c = peek_q();
        pop_q();
        assert(c != NULL);
        
        if(c->visited == true) continue;
        
        fprintf(fp, "\ncurr %d,%d: ", c->x,c->y);
        // Get all possible neighbors
        struct cell *nbr_n = get_nbr(_n, c);
        struct cell *nbr_e = get_nbr(_e, c);
        struct cell *nbr_s = get_nbr(_s, c);
        struct cell *nbr_w = get_nbr(_w, c);

        struct cell *nbrs[4];
        int nbr_cnt = 0;
        if (nbr_n != NULL && !nbr_n->visited /*&& !is_processed(nbr_n->x, nbr_n->y)*/) {
            if(nbr_n->value > c->value + 1){
                nbr_n->value = c->value + 1;
            }
            nbrs[nbr_cnt++] = nbr_n;
        }
        if (nbr_e != NULL && !nbr_e->visited /*&& !is_processed(nbr_e->x, nbr_e->y)*/) {
            if(nbr_e->value > c->value + 1){
                nbr_e->value = c->value + 1;
            }
            nbrs[nbr_cnt++] = nbr_e;
        }
        if (nbr_s != NULL && !nbr_s->visited /*&& !is_processed(nbr_s->x, nbr_s->y)*/) {
            if(nbr_s->value > c->value + 1){
                nbr_s->value = c->value + 1;
            }
            nbrs[nbr_cnt++] = nbr_s;
        }
        if (nbr_w != NULL && !nbr_w->visited /*&& !is_processed(nbr_w->x, nbr_w->y)*/) {
            if(nbr_w->value > c->value + 1){
                nbr_w->value = c->value + 1;
            }
            nbrs[nbr_cnt++] = nbr_w;
        }

        // sort nbr list by value 
        dijkstra_sort_nbrs(nbrs, nbr_cnt);
        for(int j=0; j<nbr_cnt; j++){
            fprintf(fp, "%d,%d,%d ", nbrs[j]->x, nbrs[j]->y, nbrs[j]->value); 
            add_q(nbrs[j]);
        }
        fprintf(fp, "\n");
        
        c->visited = true;
    }
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
            break;
        case _s:
            is_wall_present = maze.cells[x][y].wbm & S;
            break;
        default:
            // _w
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

int get_direction_input(const int c) {
    int rc = 1;
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
      rc = 0;
      break;
  }
  return rc;
}

void reset_maze()
{
    // Reset the values and visited status
    // to redraw the flood values
    for (int i = 0; i < MAZE_SIZE; i++) {
        for (int j = 0; j < MAZE_SIZE; j++) {
            maze.cells[i][j].value = 255;
            maze.cells[i][j].visited = false;
        }
    }
}

int manual_move()
{
    int c = getch();

    // Escape key = 27
    if (c == KEY_ESC) return -1;

    // Get manual user movement in maze
    int f_flood = get_direction_input(c);

    if(f_flood) {
        reset_maze();
        dfs(mm_pose.x, mm_pose.y, 8, 8);
    }
    return 0;
}

dir get_nbr_relative_dir(struct cell *nbr, struct cell *c)
{
   dir d;
   if(nbr->x == c->x && nbr->y == c->y - 1) d = _n;
   else if(nbr->x == c->x + 1 && nbr->y == c->y) d = _e;
   else if(nbr->x == c->x - 1 && nbr->y == c->y) d = _w;
   else if(nbr->x == c->x && nbr->y == c->y + 1) d = _s;

   return d;
}

int auto_move()
{
    //sleep(1);
    usleep(200000);
    // get reference of current cell
    struct cell *c = &maze.cells[mm_pose.x][mm_pose.y];
    //fprintf(fp, "curr %d,%d: ", c->x,c->y);
    c->phy_visited = 1;

    reset_maze(); 
    //bfs(mm_pose.x, mm_pose.y, 8, 8);
    //dfs(mm_pose.x, mm_pose.y, 8, 8);
    dijkstra(mm_pose.x, mm_pose.y, 8, 8);

    // get nbrs, move to non visited, min val open nbr
    struct cell *nbrs[4];
    int nbr_cnt = get_nbrs(nbrs, c);  

    fprintf(fp,",nbr_cnt %d", nbr_cnt); 
    for(int k=0;k<nbr_cnt;k++){ fprintf(fp,",(%d,%d)",nbrs[k]->x,nbrs[k]->y); }
    fprintf(fp,"\n");

    // sort nbr list by value, phy_visited etc
    sort_nbrs(nbrs, nbr_cnt);
    for(int j=0; j<nbr_cnt; j++){
        fprintf(fp, "%d,%d,%d ", nbrs[j]->x, nbrs[j]->y, nbrs[j]->value); 
    }
    fprintf(fp, "\n");

    // get relative direction of min nbr based on its x,y
    dir d = get_nbr_relative_dir(nbrs[0], c);    

    // move to min nbr
    fprintf(fp, "curr %d,%d -> nxt %d,%d @ %d", c->x, c->y, nbrs[0]->x, nbrs[0]->y, d);
    make_pose_update(d); 

    if(c->x == 7 && c->y == 7 
    || c->x == 7 && c->y == 8
    || c->x == 8 && c->y == 7 
    || c->x == 8 && c->y == 8 ){
        while(1);
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[]) {

  initscr();
  cbreak();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);
  scrollok(stdscr, TRUE);
  noecho();
  getmaxyx(stdscr, max_y, max_x);
  curs_set(FALSE);

  srand(time(NULL));


  // Initialization of maze
  for (int i = 0; i < MAZE_SIZE; i++) {
    for (int j = 0; j < MAZE_SIZE; j++) {
      maze.cells[i][j].x = i;
      maze.cells[i][j].y = j;
      maze.cells[i][j].value = 255;
      maze.cells[i][j].visited = false;
    }
  }
  static char s;

  int f_flood = 0;

  draw_maze();
  draw_maze_actual();

  fp = fopen("log.txt","w");
  time_t rawtime; 
  struct tm *info;
  char timebuf[80];
  time(&rawtime);
  info = localtime(&rawtime);
  strftime(timebuf,80, "%Y-%m-%d_%I:%M:%p", info);
  fprintf(fp, "%s %s\n", timebuf,"starting new***************************************");
  while (1) {
    erase();

    // Simulate reading from a real sensor
    short newwall = discover_walls(mm_pose.x, mm_pose.y);
    set_walls(mm_pose.x, mm_pose.y, newwall);
    
    int ret = auto_move(); 
    //int ret = manual_move();
    if(ret == -1) break;
     

    draw_maze();
    draw_maze_actual();
    get_center(mm_pose.x, mm_pose.y, &mm_pose.mx, &mm_pose.my);
    s = get_mouse_symbol(mm_pose.curr_direction);
    mvprintw(mm_pose.my, mm_pose.mx, &s);

    refresh();
    usleep(DELAY_MILLIS);
  }
    getch();
  fclose(fp);
  endwin(); // Restore normal terminal behavior
}
