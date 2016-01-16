#define _BSD_SOURCE
#include <unistd.h>

#include <errno.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "communication.h"
#include "graphX.h"

int sound_queue;
//client-variables
Player c_player = {{MX/2, MY-2}, 0, 5, 0, 1, 0};
Object c_obj[MX * MY] = {{{0,0}, 0, 0, NO_CHANGE}};
char *client_data_exchange_container = NULL;
char client_send_buf = 0;
Shot c_shots[AMUNITION] = { {{0, 0}, 0} };
char playername[PLAYER_NAME_LEN + 1] = "";

//clientside functions
int connect2server(char ip[16], int port);
void init_shot(Player *_player, int input);
void move_player(Player *_player, int input);
void gameloop(int gamesocket);

int main(int argc, char **argv) {
	int gamesocket;
	int port = STD_PORT;
	char ip[16] = "127.0.0.1";
	char playername[PLAYER_NAME_LEN + 1] = "";
	int ch;
	int mode = 0;


 sound_queue = open("S_QUEUE", O_RDWR);

	// Check arguments
	for(int i = 1; i < argc; i = i+2){
		if(strcmp(argv[i], "-i") != 0 && strcmp(argv[i], "-p") != 0 && strcmp(argv[i], "-n") != 0) error_handler(-21);		//Check if a wrong flag was entered
		if(strcmp(argv[i], "-i") == 0 && i+1 < argc) strcpy(ip, argv[i+1]);
		if(strcmp(argv[i], "-p") == 0 && i+1 < argc) port = atoi(argv[i+1]);			//Convert string argument to int
		if(strcmp(argv[i], "-n") == 0 && i+1 < argc){
			if(strlen(argv[i+1]) <= PLAYER_NAME_LEN) strcpy(playername, argv[i+1]);			//Check if not too long
			else error_handler(ERR_PLAYERNAME);
		}
	}
	if(port <= PORT_MIN || port >= PORT_MAX) error_handler(-2);

	//Connect
	gamesocket = connect2server(ip, port);
	if(gamesocket < 0) error_handler(gamesocket);

  // Send gamemode (new game/ spectator)
	if(strlen(playername) != 0){ //wants to start a new game
		mode = NEWGAME;
		ret = send(gamesocket, &mode, sizeof(int), 0);
		if(ret < 0) error_handler(-7);
		msgSize = recv(gamesocket, &port, sizeof(int), 0); 							//TODO:  sizeof correct?
		if(msgSize <= 0) error_handler(-8);
		close(gamesocket);
		gamesocket = connect2server(ip, port);
		//Send playername to server
		ret = send(gamesocket, playername, sizeof(playername), 0);
		if(ret < 0) error_handler(-7);

	}
	else{ //wants to become a spectator
		ret = send(gamesocket, &mode, sizeof(int), 0);
		if(ret < 0) error_handler(-7);
		//msgSize = recv(gamesocket, XX, XX), 0);						//recive struct with ongoing games
		close(gamesocket);
		/*
		 * ask user which game (set port)
		 */
		gamesocket = connect2server(ip, port);
		if(gamesocket < 0) error_handler(gamesocket);
		/*
		 * adapt code for spectator
		 */
		exit(EXIT_SUCCESS);
	}


	gameloop(gamesocket);
	return 0;
}

void gameloop(int gamesocket){
	int ret = 0;
	int msgSize = -1;
	int ch;

	//Send playername to server
	ret = send(gamesocket, playername, sizeof(playername), 0);
	if(ret < 0) error_handler(-7);

// GAME STARTS HERE ------------------------------------------------
	  client_data_exchange_container = malloc(SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
	  //memset(client_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
	  init_graphix();
	  print_scorescr(playername, c_player.score, c_player.life, 0);		// TODO: change from 0 to number of spectators!
	  usleep(DELAY);

//BEGIN MAIN LOOP-------------------------------------------------------------
	while(1) {
	//clientside -> start

		//GET TCP PACKAGE
		//memset(client_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
		msgSize = recv(gamesocket, client_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER, 0);

		if(msgSize <= 0){
			if(errno != EWOULDBLOCK)error_handler(-8);
		}

		//Look if player is game over
		if(((Player*)client_data_exchange_container)->life == 0){
			ret = disp_infoscr('g');
			if(ret == 'y'){					//really exit
				c_player.instructions |= QUIT;
				ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
					if(ret < 0) error_handler(-7);
				break;
			}
		}

		mvwprintw(statscr, 1, 8, "%u ; %u", ((Player*)client_data_exchange_container)->pos[0], ((Player*)client_data_exchange_container)->pos[1] );

		//DECODE TRANSMITTED PACKAGE
		if(errno != EWOULDBLOCK) handle_package(client_data_exchange_container, &c_player, c_obj, c_shots, DISASSEMBLE);
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
		print_scorescr(playername, c_player.score, c_player.life, 0);		// TODO: change from 0 to number of spectators!

		//Delay to reduce cpu-load
		//TODO: time accurately to a certain number of updates per second
		usleep(DELAY);

		//read user-input
		ch = wgetch(fieldscr);

		move_player(&c_player, ch);

		init_shot(&c_player, ch);

		if(ch == 'q'){						//quit game
			ret = disp_infoscr(ch);
			if(ret == 'y'){					//really exit
				c_player.instructions |= QUIT;
				ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
					if(ret < 0) error_handler(-7);
				continue;
				break;
			}
			else init_graphix();			//just redraw screen
		}
		if(ch == 'p') disp_infoscr(ch);		//game paused
		if(ch == 'r'){						//restart game
			ret = disp_infoscr(ch);
			if(ret == 'y'){			//really exit
				c_player.instructions |= RESTART;
				ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
					if(ret < 0) error_handler(-7);
				continue;
		//TODO: add function to restart gameloop
			}
			else init_graphix();			//just redraw screen
		}

		//wrefresh(statscr);

	//TRANSMIT TCP PACKAGE
		ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
		if(ret < 0) error_handler(-7);

	//clientside <- end
	}
// GAME ENDS HERE --------------------------------------------------

  beep();
  free(client_data_exchange_container);
  endwin();

	// Disconnect from server
	ret = close(gamesocket);
	if(ret < 0) error_handler(-29);

	return;
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
			if(SOUND){
//pipe some sounds!
			}
      break;
    default:
      break;
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
				//if(obj[index].life <= 0 && obj[index].status == UPDATED && SOUND)system(PLAYME_EXPLOSION);
				if(obj[index].life <= 0 && obj[index].status == UPDATED && SOUND){
					if (sound_queue == -1) {
			        beep();
			    }else{
							write(sound_queue, "hit", 4);
					}

				}

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
