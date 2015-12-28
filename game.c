#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define DELAY 30000

#define MX 50
#define MY 10

#define AMUNITION 1

typedef struct {
  char width;
  char height;
  char* symbols;
}Art;

typedef struct {
  int pos[2];
  int type;
  int life;
  Art* art;
}Object;

typedef struct {
  int pos[2];
  int modifier;
  int life;
  int amunition;
}Player;

typedef struct{
  int pos[2];
  int active;
}Shot;

Shot shots[AMUNITION] = { {{0, 0}, 0} };

int tmp_obstacle[2] = {3,5};

void shoot(Shot _shots[AMUNITION] ,int init_pos[2], Object obj[MX * MY]);

int update_player(Player *_player, Object obj[MX * MY], int max_x, int max_y, int input);
int test_for_collision(int pos1[2], int pos2[2], int planned_step_x, int planned_step_y);

int main(int argc, char *argv[]) {
  int x = 0, y = 0;
  int max_y = 0, max_x = 0;
  int next_x = 0;
  int next_y = 0;
  int direction = 1;
  int direction_y = 1;
  int ch;

  time_t t;
  char sym[] = {'_','_','_','|','X','|','-','-','-'};

  Art art_1 = {3, 3,sym};

  Player player = {{0,0}, 0, 5, 5};
  Object obj[MX * MY] = {{{0,0}, 0, 0, &art_1}};

  //for(int i = 0; i < MX * MY; i++) obj[i].life = 0;

  srand((unsigned) time(&t));

  for(int i = 0; i < 10; i++){
    obj[i].pos[0] = rand() % MX;
    obj[i].pos[1] = rand() % MY;

    obj[i].life = rand() % 4;
  }


  initscr();
  noecho();
  cbreak();
  keypad(stdscr, TRUE);
  curs_set(FALSE);
  int toggle = 1;
  timeout(0);
  // Global var `stdscr` is created by the call to `initscr()`

  while(1) {

    //manage window
    resizeterm(MY+2, MX+2);
    getmaxyx(stdscr, max_y, max_x);
    clear();
    wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');
    mvprintw(player.pos[1] + 1, player.pos[0] + 1, "o");


    for(int i = 0; i < MX * MY; i++)if(obj[i].life > 0)mvprintw(obj[i].pos[1] + 1, obj[i].pos[0] + 1, "X");

    //simple frame-change indicator
    if(toggle){
      mvprintw(0, 0, "+");
      toggle = 0;
    }else{
      mvprintw(0, 0, "-");
      toggle = 1;
    }

    //update shots
    shoot(shots, NULL, obj);

    //TODO: Update all moving objects and detect collisions

    refresh();

    //Delay to reduce cpu-load
    //TODO: time accurately to a certain number of fps
    usleep(DELAY);

    //read user-input
    ch = wgetch(stdscr);

    update_player(&player, obj, max_x, max_y, ch);

    //nitiate shot
    if(ch == 's')shoot(shots, player.pos, obj);




  }

  endwin();
}

int test_for_collision(int pos1[2], int pos2[2], int planned_step_x, int planned_step_y){
  if(!(pos1[0] + planned_step_x == pos2[0] && pos1[1] + planned_step_y == pos2[1])) return 0;
  return 1;
}

int test_for_collision_with_object(int pos1[2], Object obj[MX * MY], int planned_step_x, int planned_step_y){

  int collision = 0;
  for(int i = 0; i < (MX * MY); i++){
      if(test_for_collision(pos1, obj[i].pos, planned_step_x, planned_step_y) && (obj[i].life > 0)) collision ++;
      if(collision) return 1;
  }

  return collision;
}

int update_player(Player *_player,Object obj[MX * MY], int max_x, int max_y, int input){

  //update player position
  if ((_player->pos[0] < (max_x - 3))&&(input == KEY_RIGHT)) {
    if(!test_for_collision_with_object(_player->pos, obj, 1, 0)) _player->pos[0]++;


  } else if ((_player->pos[0]  > 0)&&(input == KEY_LEFT)) {
    if(!test_for_collision_with_object(_player->pos, obj, -1, 0)) _player->pos[0]--;
  }

  if ((_player->pos[1] < (max_y - 3))&&(input==KEY_DOWN)) {
    if(!test_for_collision_with_object(_player->pos, obj, 0, 1)) _player->pos[1]++;
  } else if((_player->pos[1]  > 0)&&(input==KEY_UP)) {
    if(!test_for_collision_with_object(_player->pos, obj, 0, -1)) _player->pos[1]--;
  }

  return 0;
}

void shoot(Shot _shots[AMUNITION] ,int init_pos[2], Object obj[MX * MY]){

  for(int i = 0; i < AMUNITION; i++){
    if(!_shots[i].active && init_pos != NULL){
      _shots[i].active = 1;
      _shots[i].pos[0] = init_pos[0];
      _shots[i].pos[1] = init_pos[1];
    }else if(_shots[i].active && obj != NULL){
      _shots[i].pos[1] -= 1;

      if((_shots[i].pos[1] < 1)||test_for_collision_with_object(_shots[i].pos, obj, 0, 0)){
          for(int o = 0; o < MX * MY; o++)if((obj[o].pos[0] ==  _shots[i].pos[0]) && (obj[o].pos[1] ==  _shots[i].pos[1])) obj[o].life--;
        _shots[i].active = 0;
      }else{
        mvprintw(_shots[i].pos[1], _shots[i].pos[0] + 1, "|");
      }
    }
  }

}
