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

//client-variables
Player c_player = {{0,0}, 0, 5, 1, 0};
Object c_obj[MX * MY] = {{{0,0}, 0, 0, 0}};
char *client_data_exchange_container = NULL;
char client_send_buf = 0;
Shot c_shots[AMUNITION] = { {{0, 0}, 0} };

WINDOW* fieldscr;
WINDOW* statscr;

//clientside functions
int connect2server(char ip[16], int port);
void init_shot(Player *_player, int input);
void move_player(Player *_player, int input);
void draw_obj(Object obj[MX * MY], char character);
void draw_player(Player *_player,  char character);
void draw_shot(Shot _shots[AMUNITION], char character);
void frame_change();

int main(int argc, char **argv) {
	int ret = 0;
	int msgSize = -1;
	int gamesocket;
	int port = STD_PORT;
	char ip[16] = "127.0.0.1";
	int ch;

	// Check arguments
	for(int i = 1; i < argc; i = i+2){
		if(strcmp(argv[i], "-i") != 0 && strcmp(argv[i], "-p") != 0 && strcmp(argv[i], "-n") != 0) error_handler(-21);		//Check if a wrong flag is entered
		if(strcmp(argv[i], "-i") == 0 && i+1 < argc) strcpy(ip, argv[i+1]);
		if(strcmp(argv[i], "-p") == 0 && i+1 < argc) port = atoi(argv[i+1]);			//Convert string argument to int
		if(strcmp(argv[i], "-n") == 0 && i+1 < argc) ret = 5;
	}
	if(port <= PORT_MIN || port >= PORT_MAX) error_handler(-2);

	//Connect
	gamesocket = connect2server(ip, port);
	if(gamesocket < 0) error_handler(gamesocket);

// GAME STARTS HERE ------------------------------------------------
	  client_data_exchange_container = malloc(SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
	  //memset(client_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);

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

	  wattron( fieldscr, COLOR_PAIR(bkg_colour));
		wattron( statscr, COLOR_PAIR(bkg_colour));

		wborder(fieldscr, '|', '|', '-', '-', '+', '+', '+', '+');
		wborder(statscr,  '|', '|', '-', '-', '+', '+', '+', '+');

	  wrefresh(fieldscr);
		wrefresh(statscr);

//BEGIN MAIN LOOP-------------------------------------------------------------
	while(1) {
	//clientside -> start

		//GET TCP PACKAGE
		memset(client_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
		msgSize = recv(gamesocket, client_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER, 0);

		if(msgSize <= 0) error_handler(-8);

		mvwprintw(statscr, 1, 8, "%u ; %u", ((Player*)client_data_exchange_container)->pos[0], ((Player*)client_data_exchange_container)->pos[1] );

		//DECODE TRANSMITTED PACKAGE
		handle_package(client_data_exchange_container, &c_player, c_obj, c_shots, DISASSEMBLE);
		//DECODE END!

		c_player.instructions = 0;

		//redraw screen

		//wclear(fieldscr);
		//wborder(fieldscr, '|', '|', '-', '-', '+', '+', '+', '+');
		//draw stuff
		draw_player(&c_player, 'o');
		draw_obj(c_obj, 'X');
		draw_shot(c_shots, '|');
		//simple frame-change indicator
		frame_change();

		wrefresh(fieldscr);

		draw_player(&c_player, ' ');
		draw_obj(c_obj, ' ');
		draw_shot(c_shots, ' ');

		//Delay to reduce cpu-load
		//TODO: time accurately to a certain number of updates per second
		usleep(DELAY);

		//read user-input
		ch = wgetch(fieldscr);

		move_player(&c_player, ch);

		init_shot(&c_player, ch);

		if(ch == 'q') break;

		wrefresh(statscr);

	//TRANSMIT TCP PACKAGE
		//c_player.instructions = 16;
		ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
		if(ret < 0) error_handler(-7);
		//printf("%d\n", c_player.instructions);

	//clientside <- end
	}
// GAME ENDS HERE --------------------------------------------------

  beep();
  free(client_data_exchange_container);
  endwin();

	// Disconnect from server
	ret = close(gamesocket);
	if(ret < 0) error_handler(-29);

	return 0;
}

void move_player(Player *_player, int input){

  switch (input){
    case KEY_RIGHT:
      _player->instructions |= RIGHT;
			mvwaddstr(statscr, 1,1,"RIGHT  ");
      break;
    case KEY_LEFT:
      _player->instructions |= LEFT;
			mvwaddstr(statscr, 1,1,"LEFT  ");
      break;
    case KEY_UP:
      _player->instructions |= UP;
			mvwaddstr(statscr, 1,1,"UP    ");
      break;
    case KEY_DOWN:
      _player->instructions |= DOWN;
			mvwaddstr(statscr, 1,1,"DOWN  ");
      break;
    default:
			mvwprintw(statscr, 1,1,"      ");
      break;
  }

}

void init_shot(Player *_player, int input){
  switch (input){
    case ' ':
      _player->instructions |= INIT_SHOT;
			if(SOUND)system(PLAYME_SHOT);
      break;
    default:
      break;
  }
}

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
				if(obj[index].life <= 0 && obj[index].status == UPDATED && SOUND)system(PLAYME_EXPLOSION);

				obj[index].status = NO_CHANGE;
      }
    }
  }
}

int connect2server(char ip[16], int port){
	int gamesocket;
	struct sockaddr_in address;
	int ret = 0;

	// Fill in connection information
	address.sin_family = AF_INET; 			//IPv4 protocol
	address.sin_port = htons(port); 		//Port number htons converts byte order
	ret = inet_aton(ip, &address.sin_addr);	//Convert address to bin

	// Create Socket		Address family: AF_INET: IPv4; Socket type: SOCK_STREAM: Stream; Protocol: 0: Standard to socket type
	gamesocket = socket(AF_INET, SOCK_STREAM, 0);
	if(gamesocket < 0) return -3;

	// Connect to server
	ret = connect(gamesocket, (struct sockaddr*)&address, sizeof(address));
	if(ret < 0) return -22;

	return gamesocket;
}
