// demo.c
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// maze file
#include "yama2002.c"

#define DELAY   20000    // 1ms tick
#define SZ  4 

#define N  (1<<0) 
#define E  (1<<1) 
#define S  (1<<2) 
#define W  (1<<3) 

int maxx,maxy;
int cellw=5,cellh=2;
FILE *fp  = NULL;

// north wall only -> 0x01
// east wall only  -> 0x02
// south wall only -> 0x04
// west wall only  -> 0x08
int cur_maze[] ={
 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
 0x06, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03,
};

typedef enum{
    _n,
    _e,
    _w,
    _s 
}dir;
struct mm{
    int x;
    int y;
    int mx;
    int my;
    dir d;
};
static struct mm mm = { .x=0,.y=(SZ-1),.d = _n}; 

char getsym(dir d)
{
    char c;
    switch(d){
    case _n:   c = '^'; break; 
    case _e:   c = '>'; break; 
    case _w:   c = '<'; break; 
    case _s:   c = 'v'; break; 
    }
    return c;
}
struct cell{
    int x;
    int y;
    short wbm;
    int visited;
    int value;
};

// goal cells: (7,7) (7,8) (8,7) (8,8)
// start cell: (0,15)
struct maze{
    struct cell cells[SZ][SZ];
};
static struct maze maze;

void rect(int offset,int x,int y,short wbm)
{
    // each cell will be  w by  h px
    int y1,  x1,  y2,  x2;

    // 0,0
    //      1,1
    //          2,2
    y1 = cellh*y + 1;//maxy/4;
    x1 = cellw*x + offset;//maxx/4; 
    y2 = y1 + cellh;
    x2 = x1 + cellw;
    

    // draw lines as per wall bitmap
    // n e w s ---> 3 2 1 0

    if(wbm & N){   // north wall
        mvhline(y1, x1, 0, x2-x1);
    }
    if(wbm & E){   // east wall
        mvvline(y1, x2, 0, y2-y1);
    }
    if(wbm & W){   // west wall
        mvvline(y1, x1, 0, y2-y1);
    }
    if(wbm & S){   // south wall
        mvhline(y2, x1, 0, x2-x1);
    }

    //          N
    //        _____
    //       |     |
    //      W|     |E
    //        _____ 
    //          S
    //
    if((wbm & N) && (wbm & W)) mvaddch(y1, x1, ACS_ULCORNER);
    if((wbm & S) && (wbm & W)) mvaddch(y2, x1, ACS_LLCORNER);
    if((wbm & N) && (wbm & E)) mvaddch(y1, x2, ACS_URCORNER);
    if((wbm & S) && (wbm & E)) mvaddch(y2, x2, ACS_LRCORNER);
}


void getcenter(int x,int y,int *dx,int *dy)
{
   *dx = /*maxx/4*/3 + cellw*x + cellw/2;
   *dy = /*maxy/4*/1 + cellh*y + cellh/2;
}

// abstraction to read sensor value and get actual wall data
// currently reads wall data from a sample maze, in actual case 
// will get wall data from sensors
short discover_walls(int x,int y)
{
    short w = 0;

    // Translation between this code's x-y orientation and wall data from maz file
    // x,y (0,0)   => 15
    // x,y (0,1)   => 14
    // x,y (0,15)  => 0
    // x,y (15,0)  => 255
    // x,y (15,15) => 240
    w = yama2002_maz[SZ-1 + SZ*x - y];
    return w;
}

// get current wall data
// maze will be initially empty, as mouse searches through the maze
// wall data will be updated
short getwalls(int x,int y)
{
    short w = 0;

    // Translation between this code's x-y orientation and wall data from maz file
    // x,y (0,0)   => 15
    // x,y (0,1)   => 14
    // x,y (0,15)  => 0
    // x,y (15,0)  => 255
    // x,y (15,15) => 240
    w = cur_maze[SZ-1 + SZ*x - y];
    return w;
}

void setwalls(int x,int y, short walls)
{
    maze.cells[x][y].wbm = walls;
    cur_maze[SZ-1 + SZ*x - y] = walls;
}

void draw_maze()
{
    for(int i = 0; i<SZ;i++){
        for(int j = 0; j<SZ;j++){
            rect(3,i,j,getwalls(i,j));
            int x,y;
            getcenter(i,j,&x,&y);
            mvprintw(y,x,"%d",maze.cells[i][j].value);
        }
    }
}

void draw_maze_actual()
{
    for(int i = 0; i<SZ;i++){
        for(int j = 0; j<SZ;j++){
            rect(85,i,j,discover_walls(i,j));
            int x,y;
            getcenter(i,j,&x,&y);
            //mvprintw(y,x,"%d",maze.cells[i][j].value);
        }
    }
}
// returns 1 if cell is valid
int check_coord_valid(int x, int y)
{
    if(x < 0 || x >= SZ || y < 0 || y >= SZ)
        return 0;
    else
        return 1;
}

// check if the cell with nx and ny is open to cell c
// returns 1 if nbr is open
int check_if_nbr_open(struct cell *c,dir nbr_dir)
{
    int walls = getwalls(c->x,c->y);
    if(nbr_dir == _n && !(walls & N)){
        return 1;
    }
    if(nbr_dir == _e && !(walls & E)){
        return 1;
    }
    if(nbr_dir == _w && !(walls & W)){
        return 1;
    }
    if(nbr_dir == _s && !(walls & S)){
        return 1;
    }

    return 0;
}

// returns pointer to valid and open nbr
struct cell *get_nbr(dir d,struct cell *p)
{
    int isvalid = 0,isopen = 0;
    int nx,ny;
    if(d == _n){
        nx = p->x; ny = p->y - 1;
    }
    if(d == _e){
        nx = p->x + 1 ; ny = p->y;
    }
    if(d == _s){
        nx = p->x; ny = p->y + 1;
    }
    if(d == _w){
        nx = p->x - 1; ny = p->y;
    }
    isvalid = check_coord_valid(nx,ny);
    isopen =  check_if_nbr_open(p,d);
    if(isvalid && isopen){
        return &maze.cells[nx][ny];
    }
    return NULL;
}

// gets nbrs of c and fills list with them
// returns number of open and valid nbrs
int get_nbrs(struct cell **list,struct cell *c)
{
    int i = 0;
    if(list == NULL || c == NULL)   return -1;

    // find valid nbrs of c
    int nbrx,nbry;
    for(dir nbdir = _n; nbdir <= _s; nbdir++){
        switch(nbdir){
        case _n: nbrx = c->x;     nbry = c->y - 1;  break;
        case _e: nbrx = c->x + 1; nbry = c->y;      break;
        case _w: nbrx = c->x - 1; nbry = c->y;      break;
        case _s: nbrx = c->x;     nbry = c->y + 1;  break;    
        }
        
        int isvalid = check_coord_valid(nbrx,nbry);
        int isopen = check_if_nbr_open(c,nbdir);
    
        if(isvalid && isopen){
            // add to nbr list
            list[i++] = &maze.cells[nbrx][nbry];
        }    
    }
    return i;
}

// sort nbr list based on value
void sort_nbrs(struct cell **list,int num)
{
    for(int i=0;i<num-1;i++){
        for(int j=0;j<num-1-i;j++){
            if((list[j])->value > (list[j+1])->value){
                struct cell *tmp = list[j];
                list[j] = list[j+1];
                list[j+1] = tmp;
            }
        }
    } 
}

// container for cell pointer queue
struct cell *arr[SZ*SZ];

struct cq{
    int r;
    int w;
    int count;
};

struct cq cq = { .r = 0, .w = 0, .count = 0 };

int q_isempty()
{
    return (cq.count == 0);
}

void add_q(struct cell *p)
{
    arr[cq.w] = p;
    mvprintw(45,0,"add x,y %d,%d",p->x,p->y);
    cq.w = (cq.w+1)%(SZ*SZ);
    cq.count++;
}

struct cell *peek_q()
{
    if(!q_isempty())
        return arr[cq.r];
    else
        return NULL;
}

void pop_q()
{
    if(!q_isempty()){
        cq.r = (cq.r+1)%(SZ*SZ);
        cq.count--;
    }
}

void q_status()
{
    mvprintw(41,0,"c %d, r %d, w %d\n",cq.count,cq.r,cq.w);
}

void reset_q()
{
    cq.w = cq.r = cq.count = 0;
}

void floodfill(int dx,int dy,int sx,int sy)
{
    // add sx,sy to queue of cell pointers
    struct cell *p = &(maze.cells[sx][sy]);
    p->value = 0;
    add_q(p);

    int cx = sx,cy = sy;

    while(!(cx == dx && cy == dy)){

        // get one cell from q to analyze its nbrs
        struct cell *tmp = peek_q();
        if(!tmp){
            mvprintw(43,0,"cx %d,cy %d, Aborting...",cx,cy);
            sleep(10);
            break;
        }else{
            pop_q();    // delete from q if peek successful
        }
        cx = tmp->x;
        cy = tmp->y;
        //tmp->visited = 1;
        fprintf(fp, "\ncx %d, cy %d, val %d: ", cx, cy, tmp->value); 
        // get valid open nbrs
        struct cell *nbr_n = get_nbr(_n,tmp);
        struct cell *nbr_e = get_nbr(_e,tmp);
        struct cell *nbr_s = get_nbr(_s,tmp);
        struct cell *nbr_w = get_nbr(_w,tmp);

        int newval = tmp->value + 1;
        if(nbr_n != NULL && nbr_n->value > newval){
            nbr_n->value = newval;
            fprintf(fp, "N(%d,%d,%d), ", nbr_n->x, nbr_n->y, nbr_n->value); 
            add_q(nbr_n);
        }
        if(nbr_e != NULL && nbr_e->value > newval){
            nbr_e->value = newval;
            fprintf(fp, "E(%d,%d,%d), ", nbr_e->x, nbr_e->y, nbr_e->value); 
            add_q(nbr_e);
        }
        if(nbr_s != NULL && nbr_s->value > newval){
            nbr_s->value = newval;
            fprintf(fp, "S(%d,%d,%d), ", nbr_s->x, nbr_s->y, nbr_s->value); 
            add_q(nbr_s);
        }
        if(nbr_w != NULL && nbr_w->value > newval){
            nbr_w->value = newval;
            fprintf(fp, "W(%d,%d,%d) ", nbr_w->x, nbr_w->y, nbr_w->value); 
            add_q(nbr_w);
        }
        fprintf(fp,"\n");
    }

    reset_q();
}

int main(int argc, char *argv[]) {

 	initscr();
 	cbreak();
 	nodelay(stdscr,TRUE);
 	keypad(stdscr, TRUE);
 	scrollok(stdscr, TRUE);
 	noecho();
    getmaxyx(stdscr,maxy,maxx);
 	curs_set(FALSE);

 	srand(time(NULL));
    fp = fopen("log", "w");    

    for(int i =0 ;i<SZ;i++){
        for(int j=0;j<SZ;j++){
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
    while(1){
        clear();
        
        int c = getch();

        switch(c){
        case KEY_UP: 
            mm.y--;
            mm.d = _n;
            f_flood = 1;
        break;  
        case KEY_RIGHT: 
            mm.x++;
            mm.d = _e;
            f_flood = 1;
        break;
        case KEY_LEFT: 
            mm.x--;
            mm.d = _w;
            f_flood = 1;
        break;
        case KEY_DOWN: 
            mm.y++;
            mm.d = _s;
            f_flood = 1;
        break;
        case ERR:
        break;
        }

        short newwall = discover_walls(mm.x,mm.y);
        setwalls(mm.x,mm.y,newwall);
        if(f_flood){
               for(int i =0 ;i<SZ;i++){
                    for(int j=0;j<SZ;j++){
                        maze.cells[i][j].value = 255;
                    }
                }
            floodfill(mm.x,mm.y,8,8);
            f_flood = 0;
        }
        

        draw_maze();
        draw_maze_actual();
        getcenter(mm.x,mm.y,&mm.mx,&mm.my);
        s = getsym(mm.d);
        mvprintw(mm.my,mm.mx,&s);

        mvprintw(40,0,"Status: ");
        mvprintw(40,10,"x: %d, y: %d",mm.x,mm.y);
        q_status();

        refresh();
        usleep(DELAY);
    }
	

	endwin(); // Restore normal terminal behavior

}

#if 0
int c = getch();

        switch(c){
        case KEY_UP: 
            mm.y--;
            mm.d = _n;
            val++;
        break;  
        case KEY_RIGHT: 
            mm.x++;
            mm.d = _e;
            val++;
        break;
        case KEY_LEFT: 
            mm.x--;
            mm.d = _w;
            val++;
        break;
        case KEY_DOWN: 
            mm.y++;
            mm.d = _s;
            val++;
        break;
        case ERR:
        break;
        }

        maze.cells[mm.x][mm.y].value = val;
        getcenter(mm.x,mm.y,&mm.mx,&mm.my);
        s = getsym(mm.d);
        mvprintw(mm.my,mm.mx-1,&s);
#endif

        
