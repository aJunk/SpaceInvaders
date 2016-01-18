/**************************************************************************
 * SPACEINVADORS GAME - graphX-libary
 * Libary for a TCP/IP based Spaceinvadors game for Linux and MacOS.
 * Provides serveral grafics functions.
 *
 * written by Philipp Gotzmann, Alexander Junk and Johannes Rauer
 * UAS Technikum Wien, BMR14
 */
#include "graphX.h"
#include <ncurses.h>
#include <sys/types.h>
#include <string.h>

WINDOW* fieldscr;
WINDOW* statscr;
WINDOW* scorescr;
WINDOW* infoscr;

void draw_line(WINDOW* scr, uint16_t height_from_bottom){
  wattron( fieldscr, COLOR_PAIR(gray_colour));
  for(int i = 1; i <= MX; i++)mvwaddch(scr, MY - height_from_bottom, i, '-');
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
  infoscr = newwin(INFO_MY , INFO_MX, SCORE_MY + MY/2 - INFO_MY/2, MX/2 - INFO_MX/2);

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

  draw_line(fieldscr, HEIGHT_OF_PLAYER_SPACE);

  wattron(fieldscr, COLOR_PAIR(bkg_colour));
  wattron(statscr, COLOR_PAIR(bkg_colour));
  wattron(scorescr, COLOR_PAIR(bkg_colour));
  wattron(infoscr, COLOR_PAIR(obj_colour));

  wborder(fieldscr, '|', '|', '-', '-', '+', '+', '+', '+');	//left right top buttom tl tr bl br
  wborder(statscr,  '|', ' ', ' ', '-', '|', '|', '+', '+');
  wborder(scorescr, ' ', ' ', '-', ' ', '+', '+', '|', '|');	//left and right border become corners (height = 2 rows)

  wrefresh(scorescr);
  wrefresh(fieldscr);
  wrefresh(statscr);
}

void print_scorescr(char playername[PLAYER_NAME_LEN + 1], int16_t score, int16_t life){
	mvwprintw(scorescr, 1, 1, "%10s %-5d Lifes: %-2d", playername, score, life);
	wrefresh(scorescr);
}

void print_statscr(char mode){
	if(mode == 'p')	mvwprintw(statscr, 0, 1, " ARROWS & SPACE      [p]ause | [r]estart | [q]uit ");
	else mvwprintw(statscr, 0, 1, " press  \"q\" to exit");
	wrefresh(statscr);
}

int disp_infoscr(char mode){
	int command = 0;

	//save actual fieldscreen
	scr_dump("fieldscreen_dump");

	//draw infoscreen
	wborder(infoscr, '|', '|', '-', '-', '+', '+', '+', '+');
	switch(mode){
		case 'q': {		//quit
			mvwprintw(infoscr, 1, 1, "   Really QUIT game?");
			mvwprintw(infoscr, 2, 1, "   Press [y] to quit");
			mvwprintw(infoscr, 3, 1, "         [n] to continue");
			wrefresh(infoscr);
			command = wgetch(infoscr);
			if(command == 'y' || command == 'n') break;		//get back to mainloop where command is handled
			else command = disp_infoscr('q');				//wrong command - display again
			break;
			}
		case 'r':{		//restart
			mvwprintw(infoscr, 1, 1, "    Really RESTART game?");
			mvwprintw(infoscr, 2, 1, "    Press [y] to restart");
			mvwprintw(infoscr, 3, 1, "          [n] to continue");
			wrefresh(infoscr);
			command = wgetch(infoscr);
			if(command == 'y' || command == 'n') break;		//get back to mainloop where command is handled
			else command = disp_infoscr('r');				//wrong command - display again
			break;
			}
		case 'p': {		//pause
			mvwprintw(infoscr, 1, 1, "        Game PAUSED");
			mvwprintw(infoscr, 2, 1, " Press any key to continue");
			wrefresh(infoscr);
			wgetch(infoscr);								//wait until a key is pressed
			break;
			}
		case 'g': {		//gameover
			mvwprintw(infoscr, 1, 1, "         GAME OVER!");
			mvwprintw(infoscr, 2, 1, "    Press [r] to restart");
			mvwprintw(infoscr, 3, 1, "          [q] to quit");
			wrefresh(infoscr);
			command = wgetch(infoscr);
			if(command == 'r' || command == 'q') break;		//restart game or exit - handled in main loop
			else command = disp_infoscr('g');				//wrong command - display again
			break;
			}
		default: break;
	}
	//Delete infoscreen
	wclear(infoscr);
	wrefresh(infoscr);

	//Restore fieldscreen
	scr_restore("fieldcreen_dump");

	return command;
}

void print_server_msg(pid_t pid, char status, char msg[], int i_info, char s_info[]){
	printf(" #%5d |", pid);
	if(status == SUCCESS) 	printf(" SUCCESS |");
	else if(status == INFO) printf(" INFO    |");
	else 					printf(" ERROR   |");
	printf(" %s", msg);
	if(i_info != 0) printf(" %d", i_info);					//if optional int or string info is given
	if(strcmp(s_info, "") != 0) printf(" %s", s_info);
	printf("\n");
}
