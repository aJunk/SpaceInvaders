#include "graphX.h"
#include <ncurses.h>
#include <sys/types.h>

WINDOW* fieldscr;
WINDOW* statscr;

void draw_obj(Object _obj[MX * MY], char character){
  wattron( fieldscr, COLOR_PAIR(obj_colour));

  for(int i = 0; i < MX * MY; i++)if(_obj[i].life > 0)mvwaddch(fieldscr, _obj[i].pos[1] + 1, _obj[i].pos[0] + 1, character);

  wattron( fieldscr, COLOR_PAIR(bkg_colour));
}

void draw_player(Player *_player, char character){
  wattron( fieldscr, COLOR_PAIR(player_colour));

  mvwaddch(fieldscr, _player->pos[1] + 1, _player->pos[0] + 1, character);

  wattron( fieldscr, COLOR_PAIR(bkg_colour));
}

void draw_shot(Shot _shots[AMUNITION], char character){
  if(_shots[0].active)mvwaddch(fieldscr, _shots[0].pos[1], _shots[0].pos[0] + 1,  character);
}

void frame_change(){
  static char toggle = 0;

  if(toggle){
    mvwaddch(fieldscr, 0, 0, '+');
    toggle = 0;
  }else{
    mvwaddch(fieldscr, 0, 0, '-');
    toggle = 1;
  }
}

void init_graphix(){
  initscr();

  //position screens...all positions from here on out are relative to local origins!
  fieldscr = newwin(MY+2, MX+2, 0, 0);
  statscr = newwin(STAT_MY , STAT_MX + 2, MY + 2, 0);

  noecho();
  cbreak();
  keypad(fieldscr, TRUE);
  curs_set(FALSE);
  wtimeout(fieldscr, 0);
  //needs to be done better...(great comment!)
  //resizeterm(MY+2, MX+2);
  //init colors
  start_color();			/* Start color 			*/
  init_pair(obj_colour, COLOR_RED, COLOR_BLACK);
  init_pair(bkg_colour, COLOR_GREEN, COLOR_BLACK);
  init_pair(player_colour, COLOR_YELLOW, COLOR_BLACK);
  init_pair(gray_colour, COLOR_WHITE, COLOR_BLACK);

  wattron( fieldscr, COLOR_PAIR(gray_colour));
  for(int i = 0; i < MX; i++)mvwaddch(fieldscr, MY - HEIGHT_OF_PLAYER_SPACE, i, '-');

  wattron( fieldscr, COLOR_PAIR(bkg_colour));
  wattron( statscr, COLOR_PAIR(bkg_colour));

  wborder(fieldscr, '|', '|', '-', '-', '+', '+', '+', '+');
  wborder(statscr,  '|', '|', '-', '-', '+', '+', '+', '+');

  wrefresh(fieldscr);
  wrefresh(statscr);
}
