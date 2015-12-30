#define _BSD_SOURCE
#include <unistd.h>

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define bkg_colour 2
#define obj_colour 1
#define player_colour 3
#define DELAY 30000

#define UP 128
#define DOWN 64
#define LEFT 32
#define RIGHT 16
#define INIT_SHOT 8

#define UPDATED 128
#define NO_CHANGE 0

#define MX 50
#define MY 10

#define AMUNITION 1

#define SET_SIZE_OF_DATA_EXCHANGE_CONTAINER sizeof(Object) * MX * MY + sizeof(Player) + sizeof(int) * MX * MY + sizeof(int) * 3

typedef struct {
  char width;
  char height;
  char* symbols;
}Art;

typedef struct {
  int pos[2];
  int type;
  int life;
  //Art* art;
  char status;
}Object;

typedef struct {
  int pos[2];
  int modifier;
  int life;
  int amunition;
  char instructions;
}Player;

typedef struct{
  int pos[2];
  int active;
}Shot;



int tmp_obstacle[2] = {3,5};

//server-variables
Player s_player = {{0,0}, 0, 5, 1, 0};
Object s_obj[MX * MY] = {{{0,0}, 0, 0, 0}};
char *server_data_exchange_container = NULL;
char *server_rec_buf = 0;
Shot s_shots[AMUNITION] = { {{0, 0}, 0} };

//client-variables
Player c_player = {{0,0}, 0, 5, 1, 0};
Object c_obj[MX * MY] = {{{0,0}, 0, 0, 0}};
char *client_data_exchange_container = NULL;
char client_send_buf = 0;
Shot c_shots[AMUNITION] = { {{0, 0}, 0} };

void frame_change();

//prototypes
int update_player(Player *_player, Object obj[MX * MY], int max_x, int max_y);

//serverside
void shoot(Shot _shots[AMUNITION] ,int init_pos[2], Object obj[MX * MY]);
int test_for_collision(int pos1[2], int pos2[2], int planned_step_x, int planned_step_y);
//to implement for server
//update_player


//clientside
void init_shot(Player *_player, int input);
void move_player(Player *_player, int input);
void draw_obj(Object obj[MX * MY]);
void draw_player(Player *_player);
void draw_shot(Shot _shots[AMUNITION]);
//to implement for client
//draw_objects
//draw player

//START OF PROGRAMM-----------------------------------------------------------

int main(int argc, char *argv[]) {
  int x = 0, y = 0;
  int max_y = 0, max_x = 0;
  int next_x = 0;
  int next_y = 0;
  int direction = 1;
  int direction_y = 1;
  int ch;

//serverside-init -> start
  //add attributes
  server_data_exchange_container = malloc(SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);

  time_t t;
  //for(int i = 0; i < MX * MY; i++) obj[i].life = 0;
  srand((unsigned) time(&t));

  for(int i = 0; i < 10; i++){
    s_obj[i].pos[0] = rand() % MX;
    s_obj[i].pos[1] = rand() % MY;

    s_obj[i].life = rand() % 4;
    s_obj[i].status = UPDATED;
  }
//serverside-init <- end

//clientside-init -> start
  client_data_exchange_container = malloc(SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
  memset(client_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);

  initscr();
  noecho();
  cbreak();
  keypad(stdscr, TRUE);
  curs_set(FALSE);
  timeout(0);
  resizeterm(MY+2, MX+2);
  getmaxyx(stdscr, max_y, max_x);
  clear();
  //TODO: Update all moving objects and detect collisions

  refresh();
  wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');
  // Global var `stdscr` is created by the call to `initscr()`

  //init colors
  start_color();			/* Start color 			*/
	init_pair(obj_colour, COLOR_RED, COLOR_BLACK);
  init_pair(bkg_colour, COLOR_GREEN, COLOR_BLACK);
  init_pair(player_colour, COLOR_YELLOW, COLOR_BLACK);

  attron(COLOR_PAIR(bkg_colour));
//clientside-init <- end

//BEGIN MAIN LOOP-------------------------------------------------------------

  while(1) {

//clientside -> start

    //DECODE TRANSMITTED PACKAGE

    char *c_tmp = client_data_exchange_container;
    memcpy(&c_player, c_tmp, sizeof(Player));
    c_tmp += sizeof(Player);
    memcpy(c_shots, c_tmp, sizeof(Shot) * AMUNITION);
    c_tmp += sizeof(Shot) * AMUNITION;
    int index = 0;
    int count = 0;

    memcpy(&count, c_tmp, sizeof(int));

    if(count > 0){
      for(int i = 0; i < count; i++){
        memcpy(&index, c_tmp + sizeof(int) + (sizeof(Object) + sizeof(int)) * i, sizeof(int));
        memcpy(&(c_obj[index]), c_tmp + sizeof(int) + sizeof(Object) * i + sizeof(int) * (i + 1), sizeof(Object));
      }
    }

    //DECODE END!




    //TODO: Update all moving objects and detect collisions

    c_player.instructions = 0;
    //redraw screen
    refresh();
    clear();
    wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');

    //draw player
    draw_player(&c_player);

    //draw all objects - TODO: change object structure to chained Lists for saving memory or make distribution of free space smarter
    draw_obj(c_obj);

    //draw shots
    draw_shot(c_shots);

    //simple frame-change indicator
    frame_change();

    //Delay to reduce cpu-load
    //TODO: time accurately to a certain number of updates per second
    usleep(DELAY);

    //read user-input
    ch = wgetch(stdscr);

    move_player(&c_player, ch);

    init_shot(&c_player, ch);

    if(ch == 'q')break;



//clientside <- end

    //memcpy(s_obj, c_obj, MX * MY * sizeof(Object));
    memcpy(&(s_player.instructions), &(c_player.instructions), sizeof(c_player.instructions));


//serverside -> start

    //initiate shot & update player
    if(s_player.instructions & INIT_SHOT)shoot(s_shots, s_player.pos, s_obj);
    update_player(&s_player, s_obj, max_x, max_y);


    //update shots
    shoot(s_shots, NULL, s_obj);


    //ENCODE NETWORK PACKAGE
    memset(server_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
    char *tmp = server_data_exchange_container;
    memcpy(tmp, &s_player, sizeof(Player));
    tmp += sizeof(Player);
    memcpy(tmp, s_shots, sizeof(Shot) * AMUNITION);
    tmp+= sizeof(Shot) * AMUNITION;
    int tmp_int = 0;
    for(int i = 0; i < MX * MY; i++){
      if(s_obj[i].status & UPDATED){
        memcpy(tmp + sizeof(int) + (sizeof(Object) + sizeof(int)) * tmp_int, &i, sizeof(int));
        memcpy(tmp + sizeof(int) + sizeof(Object) * tmp_int + sizeof(int) * (tmp_int + 1), &(s_obj[i]), sizeof(Object));
        tmp_int++;
        s_obj[i].status = NO_CHANGE;
      }
    }
    memcpy(tmp, &tmp_int, sizeof(int));
    tmp = NULL;

    //ENCODE END!

    //TRANSMIT TCP PACKAGE
    memcpy(client_data_exchange_container, server_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);

//serverside <- end



  }

  beep();
  free(server_data_exchange_container);
  free(client_data_exchange_container);

  endwin();
}

//END OF PROGRAMM--------------------------------------------------------------
void move_player(Player *_player, int input){

  switch (input){
    case KEY_RIGHT:
      _player->instructions |= RIGHT;
      break;
    case KEY_LEFT:
      _player->instructions |= LEFT;
      break;
    case KEY_UP:
      _player->instructions |= UP;
      break;
    case KEY_DOWN:
      _player->instructions |= DOWN;
      break;
    default:
      break;
  }

}

void init_shot(Player *_player, int input){
  switch (input){
    case 's':
      _player->instructions |= INIT_SHOT;
      break;
    default:
      break;
  }
}

void draw_obj(Object _obj[MX * MY]){
  attron(COLOR_PAIR(obj_colour));

  for(int i = 0; i < MX * MY; i++)if(_obj[i].life > 0)mvprintw(_obj[i].pos[1] + 1, _obj[i].pos[0] + 1, "X");

  attron(COLOR_PAIR(bkg_colour));
}

void draw_player(Player *_player){
  attron(COLOR_PAIR(player_colour));

  mvprintw(_player->pos[1] + 1, _player->pos[0] + 1, "o");

  attron(COLOR_PAIR(bkg_colour));
}

void draw_shot(Shot _shots[AMUNITION]){
  if(_shots[0].active)mvprintw(_shots[0].pos[1], _shots[0].pos[0] + 1, "|");
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

int update_player(Player *_player,Object obj[MX * MY], int max_x, int max_y){

  //update player position
  if ((_player->pos[0] < (max_x - 3))&&(_player->instructions & RIGHT)) {
    if(!test_for_collision_with_object(_player->pos, obj, 1, 0)) _player->pos[0]++;


  } else if ((_player->pos[0]  > 0)&&(_player->instructions & LEFT)) {
    if(!test_for_collision_with_object(_player->pos, obj, -1, 0)) _player->pos[0]--;
  }

  if ((_player->pos[1] < (max_y - 3))&&(_player->instructions & DOWN)) {
    if(!test_for_collision_with_object(_player->pos, obj, 0, 1)) _player->pos[1]++;
  } else if((_player->pos[1]  > 0)&&(_player->instructions & UP)) {
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
          for(int o = 0; o < MX * MY; o++)if((obj[o].pos[0] ==  _shots[i].pos[0]) && (obj[o].pos[1] ==  _shots[i].pos[1])){
            obj[o].life--;
            obj[o].status = UPDATED;
          }
        _shots[i].active = 0;
      }else{
        //mvprintw(_shots[i].pos[1], _shots[i].pos[0] + 1, "|");
      }
    }
  }

}

void frame_change(){
  static char toggle = 0;

  if(toggle){
    mvprintw(0, 0, "+");
    toggle = 0;
  }else{
    mvprintw(0, 0, "-");
    toggle = 1;
  }
}
