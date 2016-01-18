/**************************************************************************
 * SPACEINVADERS GAME - CLIENT
 * Client-program for a TCP/IP based Spaceinvaders game for Linux and MacOS.
 * User can either start a new game or become a spectator of an ongoing game.
 *
 * written by Philipp Gotzmann, Alexander Junk and Johannes Rauer
 * UAS Technikum Wien, BMR3_2014
 */
#ifdef __linux__
#define _POSIX_C_SOURCE 199309L
#endif

#define _BSD_SOURCE

#include <unistd.h>
#include <errno.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include "communication.h"
#include "graphX.h"
#include "client.h"

//client-variables
Player c_player = {{MX/2, MY-2}, 0, 5, 0, 1, 0};
Object c_obj[MX * MY] = {{{0,0}, 0, 0, NO_CHANGE}};
uint8_t *client_data_exchange_container = NULL;
char client_send_buf = 0;
Shot c_shots[AMUNITION] = { {{0, 0}, 0} };
char playername[PLAYER_NAME_LEN + 1] = "";
int sound_queue;

int main(int argc, char **argv) {
	int gamesocket;
	int ret;
	int port = STD_PORT;
	char ip[16] = "127.0.0.1";
	char ch = 0;
	uint8_t role = 0;
	int msgSize;
	Game tmp_game_mem[MAXGAMES]={{{""},0,0,0}};

	struct sigaction sig = {.sa_handler = sig_handler};
	sigemptyset(&sig.sa_mask);
	sigaction(SIGINT, &sig, NULL);
	sigaction(SIGPIPE, &sig, NULL);

 sound_queue = open("S_QUEUE", O_RDWR);

	// Check arguments
	for(int i = 1; i < argc; i = i+2){
		if(strcmp(argv[i], "-i") != 0 && strcmp(argv[i], "-p") != 0 && strcmp(argv[i], "-n") != 0) error_handler(ERR_INVALID_ARGUMENT, EXIT);		//Check if a wrong flag was entered
		if(strcmp(argv[i], "-i") == 0 && i+1 < argc) strcpy(ip, argv[i+1]);
		if(strcmp(argv[i], "-p") == 0 && i+1 < argc) port = atoi(argv[i+1]);			//Convert string argument to int
		if(strcmp(argv[i], "-n") == 0 && i+1 < argc){
			if(strlen(argv[i+1]) <= PLAYER_NAME_LEN) strcpy(playername, argv[i+1]);			//Check if not too long
			else error_handler(ERR_PLAYERNAME, EXIT);
		}
	}
	if(port <= PORT_MIN || port >= PORT_MAX) error_handler(ERR_INVALID_PORT, EXIT);

	//Connect
	gamesocket = connect2server(ip, port);
	if(gamesocket < 0) error_handler(ERR_CONNECT, EXIT);

	init_graphix();
	wrefresh(fieldscr);

	char greeting[30] = "...connecting to server!";
	char info[30] = "please wait (may be busy)";
	mvwprintw(scorescr, 1, MX/2 - strlen(greeting)/2, greeting);
	mvwprintw(statscr, 0, MX/2 - strlen(info)/2, info);
	wrefresh(scorescr);
	wrefresh(statscr);

	msgSize = recv(gamesocket, &tmp_game_mem, sizeof(Game) * MAXGAMES, 0);
	if(msgSize <= 0){
		close (gamesocket);
		error_handler(ERR_RECV, EXIT);
	}
	for(int i=0; i<MAXGAMES; i++){
		if(tmp_game_mem[i].pid != 0){
			mvwprintw(fieldscr, 2 + i, 1, "%d : \"%s\"\n", i, tmp_game_mem[i].name);
		}else{
			mvwprintw(fieldscr, 2 + i, 1, "%d : EMPTY-SLOT", i);
		}
	}
	wrefresh(fieldscr);


	strcpy(greeting,"  WELCOME TO THE ARENA!  ");
	strcpy(info,"enter number, \"n\" for new");
	mvwprintw(scorescr, 1, MX/2 - strlen(greeting)/2, greeting);
	mvwprintw(statscr, 0, MX/2 - strlen(info)/2, info);
	wrefresh(scorescr);
	wrefresh(statscr);

	//print stuff
	timeout(1);
	do{
		ch = wgetch(fieldscr);
	}while(ch != 'n' && ((ch - 48) > (MAXGAMES-1) || (ch - 48) < 0));

	if(ch == 'n'){
		role = ACTIVE_PLAYER;
		if(strlen(playername)==0) strcpy(playername,"ANON");
	}else{ //SPECTATOR
		role = SPECTATOR;
		//set choosen port
		port=tmp_game_mem[ch - 48].port;
		strcpy(playername,""); //send empty playername
	}
	endwin();

	//Send playername to server
	ret = send(gamesocket, playername, PLAYER_NAME_LEN + 1, 0);
	if(ret < 0) error_handler(ERR_SEND, EXIT);

	uint16_t buf = 0;

	switch(role){
		case ACTIVE_PLAYER:
			//recive new gameport
			msgSize = recv(gamesocket, &buf , sizeof(uint16_t), 0);
			if(msgSize <= 0){
				close (gamesocket);
				error_handler(ERR_RECV, EXIT);
			}
			close(gamesocket);

			//Connect
			gamesocket = connect2server(ip, buf);
			if(gamesocket < 0) error_handler(gamesocket, EXIT);

			//final handshake
			buf = 0;
			msgSize = recv(gamesocket, &buf , sizeof(uint16_t), 0);
			if(msgSize <= 0){
				close (gamesocket);
				error_handler(ERR_RECV, EXIT);
			}
			//enter gameloop
			gameloop(gamesocket);
			break;
		default:
			close(gamesocket);
			//conect to game as spectator
			gamesocket = connect2server(ip, port);
			if(gamesocket < 0) error_handler(ERR_CONNECT, EXIT);

			spectate(gamesocket, tmp_game_mem[ch - 48].name);
			break;
	}

	return 0;
}

void gameloop(int gamesocket){
	int ret = 0;
	int msgSize = -1;
	int ch;
	uint8_t goon = 1;



  // GAME STARTS HERE ------------------------------------------------
	  if(client_data_exchange_container == NULL) client_data_exchange_container = malloc(SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
	  //memset(client_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
	  init_graphix();
	  print_scorescr(playername, c_player.score, c_player.life);
	  print_statscr('p');
	  usleep(DELAY);

  //BEGIN MAIN LOOP-------------------------------------------------------------
	while(goon) {

		//GET TCP PACKAGE
		msgSize = recv(gamesocket, client_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER, 0);
		if(msgSize <= 0){
			if(errno != EWOULDBLOCK)error_handler(ERR_RECV, 0);
			goon = 0;
			continue;
		}

		//Look if player is game over
		if(((Player*)client_data_exchange_container)->life == 0){
			ret = disp_infoscr('g');
			if(ret == 'q'){					//really exit
				c_player.instructions |= QUIT;
				ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
				if(ret < 0) error_handler(ERR_SEND, 0);
				goon = 0;
				continue;
			}
			else if(ret == 'r'){			//start new game
				c_player.instructions |= RESTART;
				ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
				if(ret < 0) error_handler(ERR_SEND, 0);
				gameloop(gamesocket);
				goon = 0;
				continue;
			}
		}

		//DECODE TRANSMITTED PACKAGE
		if(errno != EWOULDBLOCK) handle_package(client_data_exchange_container, &c_player, c_obj, c_shots, DISASSEMBLE);
		//DECODE END!

		c_player.instructions = 0;

		//redraw screen

		//wclear(fieldscr);
		//wborder(fieldscr, '|', '|', '-', '-', '+', '+', '+', '+');
		//draw stuff
		draw_line(fieldscr, HEIGHT_OF_PLAYER_SPACE);
		draw_player(&c_player, 'o');
		draw_obj(c_obj, 'X');
		draw_shot(c_shots, '|');
		//simple frame-change indicator
		frame_change();

		wrefresh(fieldscr);

		draw_player(&c_player, ' ');
		draw_obj(c_obj, ' ');
		draw_shot(c_shots, ' ');
		print_scorescr(playername, c_player.score, c_player.life);

		//Delay to reduce cpu-load
		//TODO: time accurately to a certain number of updates per second
		usleep(DELAY);

		//read user-input
		ch = wgetch(fieldscr);

		move_player(&c_player, ch);

		init_shot(&c_player, ch);

		//check player commands
		if(ch == 'q'){							//quit game
			ret = disp_infoscr('q');
			if(ret == 'y'){						//really exit
				c_player.instructions |= QUIT;
				ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
				if(ret < 0) error_handler(ERR_SEND, 0);
				goon = 0;
				continue;
			}
		}
		else if(ch == 'p') disp_infoscr('p');		//game paused
		else if(ch == 'r'){						//restart game
			ret = disp_infoscr('r');
			if(ret == 'y'){					//really restart
				c_player.instructions |= RESTART;
				ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
				if(ret < 0){
					error_handler(ERR_SEND, 0);
					goon = 0;
					continue;
				}
				gameloop(gamesocket);
				return;
			}
		}

	//TRANSMIT TCP PACKAGE
		ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
		if(ret < 0){
			error_handler(ERR_SEND, 0);
			goon = 0;
			continue;
		}

	//clientside <- end
	}
  // GAME ENDS HERE --------------------------------------------------

  beep();
	if(client_data_exchange_container != NULL){
		free(client_data_exchange_container);
		client_data_exchange_container=NULL;
	}
  endwin();

	// Disconnect from serverterminal
	ret = close(gamesocket);
	if(ret < 0) error_handler(ERR_CLOSING_SOCKET_FAILED, EXIT);

	return;
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
			if(SOUND){
  //pipe some sounds!
			}
      break;
    default:
      break;
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
	if(gamesocket < 0) return ERR_CREATE_SOCKET;

	// Connect to server
	ret = connect(gamesocket, (struct sockaddr*)&address, sizeof(address));
	if(ret < 0) return ERR_CONNECT;

	return gamesocket;
}

void spectate(int socket, char playername[]){
	int ret = 0;
	int msgSize = -1;
	int ch;
	uint8_t goon = 1;
	uint8_t tmp_byte = 0;

  // GAME STARTS HERE ------------------------------------------------
  client_data_exchange_container = malloc(SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
  init_graphix();


  print_scorescr(playername, c_player.score, c_player.life);
  print_statscr('s');
  usleep(DELAY);

	//request first packet
	tmp_byte = ACK;
	send(socket, &tmp_byte, sizeof(tmp_byte), MSG_DONTWAIT);

  //BEGIN MAIN LOOP-------------------------------------------------------------
	while(goon) {

		ch = wgetch(fieldscr);

		//quit game?
		if(ch == 'q'){
			ret = disp_infoscr(ch);
			if(ret == 'y'){
			 	tmp_byte = ENDOFCON;
				send(socket, &tmp_byte, sizeof(tmp_byte), 0);				//really exit
				goon = 0;
				continue;
			}
		}

		//GET TCP PACKAGE
		msgSize = recv(socket, client_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER, 0);
		//acknowloedge that we are ready to receive!
		tmp_byte = ACK;
		ret = send(socket, &tmp_byte, sizeof(tmp_byte), MSG_DONTWAIT);

		if(msgSize <= 0 || ret <= 0){
			if(errno != EWOULDBLOCK){
				error_handler(ERR_CONNECTION_LOST, 0);
				goon = 0;
				continue;
			}else{
				usleep(500000);
				continue;
			}
		}

		//Look if player is game over
		if(((Player*)client_data_exchange_container)->life == 0){
				c_player.instructions |= QUIT;
				goon = 0;
				continue;
		}

		//DECODE TRANSMITTED PACKAGE
		if(errno != EWOULDBLOCK) handle_package(client_data_exchange_container, &c_player, c_obj, c_shots, DISASSEMBLE);
		//DECODE END!

		c_player.instructions = 0;

		draw_line(fieldscr, HEIGHT_OF_PLAYER_SPACE);
		draw_player(&c_player, 'o');
		draw_obj(c_obj, 'X');
		draw_shot(c_shots, '|');
		frame_change();
		wrefresh(fieldscr);
		draw_player(&c_player, ' ');
		draw_obj(c_obj, ' ');
		draw_shot(c_shots, ' ');
		print_scorescr(playername, c_player.score, c_player.life);
		usleep(DELAY);
	}

  beep();
  if(client_data_exchange_container != NULL){
		free(client_data_exchange_container);
		client_data_exchange_container=NULL;
	}
	ret = close(socket);
	if(ret < 0) error_handler(ERR_CLOSING_SOCKET_FAILED, EXIT);
  endwin();

	return;

}

void sig_handler(){	  //if a user/system interrupts
	endwin();
	printf("*** Client ended due to interrupt ***\n");		//printf may be interrupted but better than don't handling case
	exit(EXIT_ERROR);
}
