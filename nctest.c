#define _BSD_SOURCE
#include <unistd.h>

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "communication.h"


int main(int argc, char **argv) {
		int ch = 0;
	  initscr();
	  noecho();
	  cbreak();
	  keypad(stdscr, TRUE);
	  curs_set(FALSE);
	  timeout(0);
	  resizeterm(MY+2, MX+2);
	  getmaxyx(stdscr, max_y, max_x);
	  clear();

	  refresh();
	  wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');
	  // Global var `stdscr` is created by the call to `initscr()`

//BEGIN MAIN LOOP-------------------------------------------------------------
	while(1) {
	//clientside -> start
		resizeterm(MY+2, MX+2);
		clear();
		wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');

		refresh();

		//Delay to reduce cpu-load
		//TODO: time accurately to a certain number of updates per second
		usleep(DELAY);

		//read user-input
		ch = wgetch(stdscr);

		if(ch == 'q') break;
		printf("%d\n", c_player.instructions);

	//clientside <- end
	}
// GAME ENDS HERE --------------------------------------------------

  beep();
  endwin();
	return 0;
}

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
    case ' ':
      _player->instructions |= INIT_SHOT;
			system("afplay ./fx/shoot-03.wav &");
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


//GET FUNCTION TO EXTERNAL FILE
void handle_package(char *container, Player *player, Object obj[MX * MY], Shot shots[AMUNITION], int mode){
	if(mode == DISASSEMBLE){
    char *c_tmp = container;
    memcpy(player, c_tmp, sizeof(Player));
    c_tmp += sizeof(Player);
    memcpy(shots, c_tmp, sizeof(Shot) * AMUNITION);
    c_tmp += sizeof(Shot) * AMUNITION;

    uint16_t index = 0;
    uint16_t count = 0;

    memcpy(&count, c_tmp, sizeof(uint16_t));
    if(count > 0){
      for(uint16_t i = 0; i < count; i++){
        memcpy(&index, c_tmp + sizeof(uint16_t) + (sizeof(Object) + sizeof(uint16_t)) * i, sizeof(uint16_t));
        memcpy(&(obj[index]), c_tmp + sizeof(uint16_t) + sizeof(Object) * i + sizeof(uint16_t) * (i + 1), sizeof(Object));

				//look for destroied objects
				if(obj[index].life <= 0)system("afplay ./fx/explosion-04.wav &");
      }
    }

  }

}
