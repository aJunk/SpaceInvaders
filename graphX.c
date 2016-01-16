#include "graphX.h"
#include <ncurses.h>
#include <sys/types.h>

WINDOW* fieldscr;
WINDOW* statscr;
WINDOW* scorescr;
WINDOW* infoscr;

void print_statscr(char* str){
  mvwprintw(statscr, 1, 1,"%s", str);
  wrefresh(statscr);
}

void draw_obj(Object _obj[MX * MY], char character){

  for(int i = 0; i < MX * MY; i++){
    if(_obj[i].life > 0 && character != ' '){
      switch(_obj[i].type){
        case 1:
          wattron( fieldscr, COLOR_PAIR(obj_colour));
          mvwaddch(fieldscr, _obj[i].pos[1] + 1, _obj[i].pos[0] + 1, character);
          break;
        case 2:
          wattron( fieldscr, COLOR_PAIR(obj_2_color));
          mvwaddch(fieldscr, _obj[i].pos[1] + 1, _obj[i].pos[0] + 1, OP_SHOT);
          break;
        default:
          mvwprintw(statscr, 1, 1, "did not display unknown object!");
          wrefresh(statscr);
          break;
      }
    }else if(_obj[i].life > 0 && character == ' '){
      mvwaddch(fieldscr, _obj[i].pos[1] + 1, _obj[i].pos[0] + 1, character);
    }
  }

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
  scorescr = newwin(SCORE_MY , SCORE_MX + 2, 0, 0);
  fieldscr = newwin(MY+2, MX+2, SCORE_MY , 0);
  statscr = newwin(STAT_MY , STAT_MX + 2, SCORE_MY + MY + 2, 0);

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
  init_pair(obj_2_color, COLOR_MAGENTA, COLOR_BLACK);

  wattron( fieldscr, COLOR_PAIR(gray_colour));
  for(int i = 0; i <= MX; i++)mvwaddch(fieldscr, MY - HEIGHT_OF_PLAYER_SPACE, i, '-');

  wattron(fieldscr, COLOR_PAIR(bkg_colour));
  wattron(statscr, COLOR_PAIR(bkg_colour));
  wattron(scorescr, COLOR_PAIR(bkg_colour));

  wborder(fieldscr, '|', '|', '-', '-', '+', '+', '+', '+');	//left right top buttom tl tr bl br
  wborder(statscr,  '|', '|', '-', '-', '+', '+', '+', '+');
  wborder(scorescr, ' ', ' ', '-', ' ', '+', '+', '|', '|');	//left and right border become corners (height = 2 rows)

  wrefresh(scorescr);
  wrefresh(fieldscr);
  wrefresh(statscr);
}

void print_scorescr(char playername[PLAYER_NAME_LEN + 1], int16_t score, int16_t life, int spectators){
	mvwprintw(scorescr, 1, 1, "%10s %-5d Lifes: %-2d   |   Spectators %2d", playername, score, life, spectators);
	wrefresh(scorescr);
}

void init_infoscr(){
	infoscr = newwin(INFO_MY , INFO_MX, SCORE_MY + MY/2 - INFO_MY/2, MX/2 - INFO_MX/2);
	wattron(infoscr, COLOR_PAIR(obj_colour));
	wborder(infoscr, '|', '|', '-', '-', '+', '+', '+', '+');
	wrefresh(infoscr);
}

//int disp_infoscr(char mode, int socket, Player *_player)
int disp_infoscr(char mode){
	int command = 0;

	init_infoscr();
	//dumb fieldscreen wborder - rest von init zu init
	switch(mode){
		case 'q': {		//quit
			mvwprintw(infoscr, 1, 1, "Really quit game?");
			mvwprintw(infoscr, 2, 1, "Press [y] to quit");
			mvwprintw(infoscr, 3, 1, "      [n] to continue");
			wrefresh(infoscr);
			command = wgetch(infoscr);
			if(command == 'y' || command == 'n') break;		//get back to mainloop where command is handled
			else disp_infoscr('q');							//wrong command - display again
			break;
			}
		case 'r':{		//restart
			mvwprintw(infoscr, 1, 1, "Really restart game?");
			mvwprintw(infoscr, 2, 1, "Press [y] to restart");
			mvwprintw(infoscr, 3, 1, "      [n] to continue game");
			wrefresh(infoscr);
			command = wgetch(infoscr);
			if(command == 'y' || command == 'n') break;		//get back to mainloop where command is handled
			else disp_infoscr('q');							//wrong command - display again
			break;
			}
		case 'p': {		//pause
			mvwprintw(infoscr, 1, 1, "Game paused");
			mvwprintw(infoscr, 2, 1, "Press any key to continue");
			wrefresh(infoscr);
			wgetch(infoscr);
			break;
			}
		case 'g': {		//gameover
			mvwprintw(infoscr, 1, 1, "Game over");
			mvwprintw(infoscr, 2, 1, "Press [r] to start a new game");
			mvwprintw(infoscr, 3, 1, "      [q] to quit");
			wrefresh(infoscr);
			command = wgetch(infoscr);
			if(command == 'r') disp_infoscr(command);			//restart game
			else if(command == 'q') command = disp_infoscr(command);		//exit
			else disp_infoscr('g');		//wrong command - display again
			break;
			}
		default: break;
	}
	init_graphix();					//let infoscreen disappear again
	//clear, wrefresh, restore fieldscreen
	return command;
}
