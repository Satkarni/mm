// Micromouse struct and constants definitions
#include "mm.h"
#include "utils.h"

#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Maze file
#include "uk2015f.c" // NOLINT(bugprone-suspicious-include)


#define DELAY_MILLIS  100000
#define KEY_ESC       27

int max_x, max_y;
int cell_width = 5, cell_height = 2;
static int goal_x = 8;
static int goal_y= 8;
static int search_seq_num;


static struct mm_pose mm_pose = {.x= 0, .y=(MAZE_SIZE - 1), .curr_direction = _n};
static struct maze maze;
FILE *fp;


struct path{
    struct cell *path[MAZE_SIZE * MAZE_SIZE];
    uint8_t len;
};
static struct path path;

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
  w = uk2015f_maz[MAZE_SIZE - 1 + MAZE_SIZE * x - y];
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
      for(int k = 0; k < path.len; k++){
        if(path.path[k]->x == i && path.path[k]->y == j){
          mvprintw(y, x, "*");
        }
      }
      //mvprintw(y, x, "%d" , maze.cells[i][j].value);
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


void dfs(int dst_x, int dst_y, int src_x, int src_y) {
    // First check validity of input args
    if (!check_coord_valid(dst_x, dst_y) || !check_coord_valid(src_x, src_y)) {
        return;
    }

    // Initialize queue w/ src cell
    stack_reset();

    // Add starting cell to queue
    struct cell *p = &(maze.cells[src_x][src_y]);
    p->value = 0;
    stack_add(p);
    int curr_sz;
    //fprintf(fp,"%s\n","stack start*****************************************");
    while (!stack_isempty()) {
        curr_sz = 1; 
        //fprintf(fp,"stack count %d\n",curr_sz);
        if(curr_sz == 0) break;

        for (int i = 0; i < curr_sz; i++) {
            // Dequeue a cell
            struct cell* curr = stack_peek();
            stack_pop();

            // Visit curr and mark its distance
            curr->visited = true;
            //fprintf(fp, "\ncurr %d,%d: ", curr->x, curr->y); 

            // Get all possible neighbors
            struct cell *nbr_n = get_nbr(_n, curr);
            struct cell *nbr_e = get_nbr(_e, curr);
            struct cell *nbr_s = get_nbr(_s, curr);
            struct cell *nbr_w = get_nbr(_w, curr);

            if (nbr_n != NULL && !nbr_n->visited && !stack_is_processed(nbr_n->x, nbr_n->y)) {
                nbr_n->value = curr->value + 1;
                stack_add(nbr_n);
                //fprintf(fp, "add %d,%d ", nbr_n->x, nbr_n->y);
            }
            if (nbr_e != NULL && !nbr_e->visited && !stack_is_processed(nbr_e->x, nbr_e->y)) {
                nbr_e->value = curr->value + 1;
                stack_add(nbr_e);
                //fprintf(fp, "add %d,%d ", nbr_e->x, nbr_e->y);
            }
            if (nbr_s != NULL && !nbr_s->visited && !stack_is_processed(nbr_s->x, nbr_s->y)) {
                nbr_s->value = curr->value + 1;
                stack_add(nbr_s);
                //fprintf(fp, "add %d,%d ", nbr_s->x, nbr_s->y);
            }
            if (nbr_w != NULL && !nbr_w->visited && !stack_is_processed(nbr_w->x, nbr_w->y)) {
                nbr_w->value = curr->value + 1;
                stack_add(nbr_w);
                //fprintf(fp, "add %d,%d ", nbr_w->x, nbr_w->y);
            }
            fflush(fp);
        }
  }
  //fprintf(fp,"\n%s\n","stack end**************************************"); 
  // Clean up after DFS is done
  stack_reset();
}

void bfs(int dst_x, int dst_y, int src_x, int src_y) {
  // First check validity of input args
  if (!check_coord_valid(dst_x, dst_y) || !check_coord_valid(src_x, src_y)) {
    return;
  }

  // Initialize queue w/ src cell
  q_reset();

  // Add starting cell to queue
  struct cell *p = &(maze.cells[src_x][src_y]);
  p->value = 0;
  q_add(p);

  int curr_sz;
  int dist = 0;
  //fprintf(fp,"%s\n","q start*****************************************");
  while (!q_isempty()) {
    curr_sz = 1; //cq.count;
    //fprintf(fp,"q count %d\n",curr_sz);
    if(curr_sz == 0) break;
    for (int i = 0; i < curr_sz; i++) {
      // Dequeue a cell
      struct cell* curr = q_peek();
      q_pop();

      // Visit curr and mark its distance
      curr->visited = true;
      curr->value = dist;
      //fprintf(fp, "\ncurr %d,%d: ", curr->x, curr->y); 
      // Get all possible neighbors
      struct cell *nbr_n = get_nbr(_n, curr);
      struct cell *nbr_e = get_nbr(_e, curr);
      struct cell *nbr_s = get_nbr(_s, curr);
      struct cell *nbr_w = get_nbr(_w, curr);

      if (nbr_n != NULL && !nbr_n->visited && !q_is_processed(nbr_n->x, nbr_n->y)) {
        q_add(nbr_n);
        //fprintf(fp, "add %d,%d ", nbr_n->x, nbr_n->y);
      }
      if (nbr_e != NULL && !nbr_e->visited && !q_is_processed(nbr_e->x, nbr_e->y)) {
        q_add(nbr_e);
        //fprintf(fp, "add %d,%d ", nbr_e->x, nbr_e->y);
      }
      if (nbr_s != NULL && !nbr_s->visited && !q_is_processed(nbr_s->x, nbr_s->y)) {
        q_add(nbr_s);
        //fprintf(fp, "add %d,%d ", nbr_s->x, nbr_s->y);
      }
      if (nbr_w != NULL && !nbr_w->visited && !q_is_processed(nbr_w->x, nbr_w->y)) {
        q_add(nbr_w);
        //fprintf(fp, "add %d,%d ", nbr_w->x, nbr_w->y);
      }
      fflush(fp);
    }
    dist += 1;
  }
  //fprintf(fp,"\n%s\n","q end**************************************"); 
  // Clean up after BFS is done
  q_reset();
}

// Sort nbr list based on value
void sort_nbrs_by_val(struct cell *list[], int num) {
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

    stack_reset();

    struct cell *c = &maze.cells[src_x][src_y];
    c->value = 0;

    // Add it to path
    q_add(c);

    while(!q_isempty()){

        struct cell *c = q_peek();
        q_pop();
        assert(c != NULL);
        
        if(c->visited == true) continue;
        
        fprintf(fp, "\ncurr %d,%d: ", c->x,c->y);

        struct cell *nbrs[4];
        int nbr_cnt = get_nbrs(nbrs, c);
        for(int j = 0; j < nbr_cnt; j++){
            if(nbrs[j] != NULL && !nbrs[j]->visited){
                if(nbrs[j]->value > c->value + 1){
                    nbrs[j]->value = c->value + 1;
                }
            } 
        }
        // sort nbr list by value 
        sort_nbrs_by_val(nbrs, nbr_cnt);
        for(int j=0; j<nbr_cnt; j++){
            fprintf(fp, "%d,%d,%d ", nbrs[j]->x, nbrs[j]->y, nbrs[j]->value); 
            q_add(nbrs[j]);
        }
        fprintf(fp, "\n");
        c->visited = true;
    }
}

// to store previous nbr 
struct cell *prev[MAZE_SIZE][MAZE_SIZE];

int astar(int dst_x, int dst_y, int src_x, int src_y, struct path *p)
{
    // First check validity of input args
    if (!check_coord_valid(dst_x, dst_y) || !check_coord_valid(src_x, src_y)) {
        return -1;
    }

    q_reset();

    struct cell *c = &maze.cells[src_x][src_y];
    assert(c);
    c->value = 0;

    // Add it to q 
    q_add(c);

    while(!q_isempty()){
        //fprintf(fp,"count: %d\n", cq.count);
        struct cell *c = q_peek();
        q_pop();
        assert(c != NULL);
        
        if(c->visited == true) continue;
        
        // destination found, reconstruct path !
        if(c->x == dst_x && c->y == dst_y){
             fprintf(fp, "%d,%d dst found, reconstructing path...\n", c->x, c->y);

             memset(&path, 0, sizeof(path));
             struct cell *itr = c;
             uint8_t idx = 0;

             // iterate through prev till src cell is reached
             do{
                 assert(itr);
                 path.path[idx++] = itr;
                 path.len++;
                 if(prev[itr->x][itr->y] != NULL){
                     itr = prev[itr->x][itr->y];
                 }else{
                     break;
                 }
             }
             while(!(itr->x == src_x && itr->y == src_y));
             fprintf(fp,"path len: %d ",path.len);
             for(int i=0; i < path.len; i++){
                fprintf(fp, "%d,%d ", path.path[i]->x,path.path[i]->y); 
             }
             fprintf(fp,"\n");
             fflush(fp);
             return 1;
        }

        //fprintf(fp, "\ncurr %d,%d: ", c->x,c->y);

        struct cell *nbrs[4];
        int nbr_cnt = get_nbrs(nbrs, c);
        int manhattan = 0;
        for(int j = 0; j < nbr_cnt; j++){
            if(nbrs[j] != NULL && !nbrs[j]->visited){
                if(nbrs[j]->value > c->value + 1){
                    nbrs[j]->value = c->value + 1;
                    // calc manhattan heuristic
                    manhattan = abs(nbrs[j]->x - dst_x) + abs(nbrs[j]->y - dst_y);  
                    nbrs[j]->value += manhattan;
                    prev[nbrs[j]->x][nbrs[j]->y] = c;
                }
            } 
        }
        // sort nbr list by value 
        sort_nbrs_by_val(nbrs, nbr_cnt);
        for(int j=0; j<nbr_cnt; j++){
            //fprintf(fp, "%d,%d,%d ", nbrs[j]->x, nbrs[j]->y, nbrs[j]->value); 
            q_add(nbrs[j]);
        }
        //fprintf(fp, "\n");
        c->visited = true;
    }
    return 0;
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
            maze.cells[i][j].value = 0xffff;
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
    usleep(50000);

    // get reference of current cell
    struct cell *c = &maze.cells[mm_pose.x][mm_pose.y];
    //fprintf(fp, "curr %d,%d: ", c->x,c->y);
    c->phy_visited = 1;

    reset_maze(); 
    memset(prev, 0, sizeof(prev));

    //bfs(mm_pose.x, mm_pose.y, 8, 8);
    //dfs(mm_pose.x, mm_pose.y, 8, 8);
    //dijkstra(mm_pose.x, mm_pose.y, 8, 8);
    /* maze exploration waypoints: start to center, then to each corner and back, then corner to corner */
    struct waypoints{
        int x;
        int y;
    };
    static struct waypoints waypoints[25] = {
        { 8,8 },
        { 0,0 },
        { 8,8 },
        { 15,0},
        { 8,8 },
        { 15,15 },
        { 8,8 },
        { 0,15 },
        { 0,0 },
        { 15,0 },
        { 15,15 },
        { 0, 15 },
        { 0, 7 },
        { 7, 0 },
        { 15,7 },
        { 7, 15 },
        { 0, 15 },
        { 8,8 } 
    };
    static int k = 0;
    if(c->x == waypoints[k].x && c->y == waypoints[k].y && search_seq_num == k){
        memset(&path, 0, sizeof(path));
        k++;
        if(k == 18) k = 16; /* Loop between start and center once exploration done */
        search_seq_num = k; 
        goal_x = waypoints[k].x;
        goal_y = waypoints[k].y;
    }

    fprintf(fp, "goal %d,%d curr %d,%d\n", goal_x, goal_y, mm_pose.x, mm_pose.y);
    astar(goal_x, goal_y, mm_pose.x, mm_pose.y, &path);

    // get relative direction of next cell based on its x,y
    struct cell *next = path.path[path.len - 1];
    assert(next);
    dir d = get_nbr_relative_dir(next, c);    

    // print path visualization
    // move to min nbr
    fprintf(fp, "curr %d,%d -> nxt %d,%d @ %d\n", c->x, c->y, next->x, next->y, d);
    make_pose_update(d); 


    return 0;
}

void status_update()
{
    int y = 35;
    mvprintw(y++,5,"cur %d,%d", mm_pose.x, mm_pose.y); 
    mvprintw(y,5,"shortest path: len %d", path.len);
    for(int i = 0; i < path.len; i++){
        mvprintw(y, 30+6*i, "%d,%d ", path.path[path.len-1-i]->x, path.path[path.len-1-i]->y);
    }
    y++;
    mvprintw(y,5,"search_seq_num %d", search_seq_num);
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

     auto_move(); 
    //manual_move();

        draw_maze();
        draw_maze_actual();
        get_center(mm_pose.x, mm_pose.y, &mm_pose.mx, &mm_pose.my);
        s = get_mouse_symbol(mm_pose.curr_direction);
        mvprintw(mm_pose.my, mm_pose.mx, &s);
        status_update();
        refresh();
        usleep(DELAY_MILLIS);
  }
  fclose(fp);
  endwin(); // Restore normal terminal behavior
}
