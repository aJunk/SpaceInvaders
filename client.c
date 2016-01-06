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
void init_shot(Player *_player, int input);
void move_player(Player *_player, int input);
void draw_obj(Object obj[MX * MY]);
void draw_player(Player *_player);
void draw_shot(Shot _shots[AMUNITION]);
void frame_change();

int main(int argc, char **argv) {
	int ret = 0;
	int msgSize = -1;
	int gamesocket;
	int port = STD_PORT;
	struct sockaddr_in address;
	char ip[16] = "127.0.0.1";

	int max_y = 0, max_x = 0;
	int ch;


// Check arguments
	if(argc > 1)
	{
		if(strcmp(argv[1], "-i") == 0) strcpy(ip, argv[2]);
		else if (strcmp(argv[1], "-p") == 0) port = atoi(argv[2]);		//Convert string argument to int
		else if (strcmp(argv[1], "-n") == 0) ret = 5;
		else ret = 99;
	}
	if(argc > 3)
	{
		if (strcmp(argv[3], "-p") == 0) port = atoi(argv[4]);
		else if (strcmp(argv[3], "-n") == 0) ret = 5;
		else ret = 99;
	}
	if(argc > 5)
	{
		if (strcmp(argv[5], "-n") == 0) ret = 5;
		else ret = 99;
	}

	if(ret == 99)
	{
		printf("ERROR: Invalid argument! Usage: client [-i <server ip>] [-p <server port>] [-n <player name>]\n");
		return EXIT_ERROR;
	}

	if(port <= PORT_MIN || port >= PORT_MAX)
	{
		printf("ERROR: Invalid port! Port has to be between %d and %d.\n", PORT_MIN, PORT_MAX);
		return EXIT_ERROR;
	}

// Fill in connection information
	address.sin_family = AF_INET; 			//IPv4 protocol
	address.sin_port = htons(port); 		//Port number htons converts byte order
	ret = inet_aton(ip, &address.sin_addr);	//Convert address to bin

// Create Socket	Address family: AF_INET: IPv4
//					Socket type: SOCK_STREAM: Stream
//					Protocol: 0: Standard to socket type
	gamesocket = socket(AF_INET, SOCK_STREAM, 0);
	if (gamesocket < 0)
	{
		perror("Error creating socket!");
		return EXIT_ERROR;
	}
	printf("Client Socket created\n");

// Connect to server
	ret = connect(gamesocket, (struct sockaddr*)&address, sizeof(address));
	if(ret < 0)
	{
		perror("Error connecting to server!");
		return EXIT_ERROR;
	}
	printf("Connected to server.\n");


// GAME STARTS HERE ------------------------------------------------
	  client_data_exchange_container = malloc(SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
	  memset(client_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);

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
		if(msgSize == 0)
		{
			perror("Error receiving, connection closed by client!");
			return EXIT_ERROR;
		}

		//DECODE TRANSMITTED PACKAGE
		handle_package(client_data_exchange_container, &c_player, c_obj, c_shots, DISASSEMBLE);
		//DECODE END!

		c_player.instructions = 0;

		//redraw screen

		//wrefresh(statscr);

		wclear(fieldscr);
		wborder(fieldscr, '|', '|', '-', '-', '+', '+', '+', '+');
		//draw stuff
		draw_player(&c_player);
		draw_obj(c_obj);
		draw_shot(c_shots);
		//simple frame-change indicator
		frame_change();

		wrefresh(fieldscr);


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
		if(ret < 0)
		{
			perror("Error sending!");
			return EXIT_ERROR;
		}
		printf("%d\n", c_player.instructions);

	//clientside <- end
	}
// GAME ENDS HERE --------------------------------------------------

  beep();
  free(client_data_exchange_container);
  endwin();

// Disconnect from server
	ret = close(gamesocket);
	if(ret < 0)
	{
		perror("Error disconnecting from server!");
		return EXIT_ERROR;
	}

	return 0;
}

void move_player(Player *_player, int input){

  switch (input){
    case KEY_RIGHT:
      _player->instructions |= RIGHT;
			mvwprintw(statscr, 1,1,"RIGHT ");
      break;
    case KEY_LEFT:
      _player->instructions |= LEFT;
			mvwprintw(statscr, 1,1,"LEFT ");
      break;
    case KEY_UP:
      _player->instructions |= UP;
			mvwprintw(statscr, 1,1,"UP   ");
      break;
    case KEY_DOWN:
      _player->instructions |= DOWN;
			mvwprintw(statscr, 1,1,"DOWN ");
      break;
    default:
			mvwprintw(statscr, 1,1,"     ");
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

void draw_obj(Object _obj[MX * MY]){
  wattron( fieldscr, COLOR_PAIR(obj_colour));

  for(int i = 0; i < MX * MY; i++)if(_obj[i].life > 0)mvwprintw(fieldscr, _obj[i].pos[1] + 1, _obj[i].pos[0] + 1, "X");

  wattron( fieldscr, COLOR_PAIR(bkg_colour));
}

void draw_player(Player *_player){
  wattron( fieldscr, COLOR_PAIR(player_colour));

  mvwprintw(fieldscr, _player->pos[1] + 1, _player->pos[0] + 1, "o");

  wattron( fieldscr, COLOR_PAIR(bkg_colour));
}

void draw_shot(Shot _shots[AMUNITION]){
  if(_shots[0].active)mvwprintw(fieldscr, _shots[0].pos[1], _shots[0].pos[0] + 1,  "|");
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
				if(obj[index].life <= 0 && obj[index].status == UPDATED && SOUND)system(PLAYME_EXPLOSION);

				obj[index].status = NO_CHANGE;
      }
    }

  }

}
