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
void spectate(int socket, char playername[]);

int main(int argc, char **argv) {
	int gamesocket;
	int ret;
	int port = STD_PORT;
	char ip[16] = "127.0.0.1";
	char ch = 0;
	uint8_t role = 0;
	int msgSize;
	Game tmp_game_mem[MAXGAMES]={{{""},0,0,0}};

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
	if(port <= PORT_MIN || port >= PORT_MAX) error_handler(ERR_INVALID_PORT);

	//Connect
	gamesocket = connect2server(ip, port);
	if(gamesocket < 0) error_handler(gamesocket);

	init_graphix();

	 msgSize = recv(gamesocket, &tmp_game_mem, sizeof(Game) * MAXGAMES, 0);
	 for(int i=0; i<MAXGAMES; i++){
		 if(tmp_game_mem[i].pid != 0){
			 mvwprintw(fieldscr, 2 + i, 1, "%d : \"%s\"\n", i, tmp_game_mem[i].name);
		 }else{
			 mvwprintw(fieldscr, 2 + i, 1, "%d : EMPTY-SLOT", i);
		 }
	 }
	wrefresh(fieldscr);

	const char greeting[] = "WELCOME TO THE ARENA!";
	const char info[] = "enter number, \"n\" for new";
	mvwprintw(scorescr, 1, MX/2 - strlen(greeting)/2, greeting);
	mvwprintw(statscr, 0, MX/2 - strlen(info)/2, info);
	wrefresh(scorescr);
	wrefresh(statscr);

	//printstuff
	timeout(1);
	do{
		ch = wgetch(fieldscr);
	}while(ch != 'n' && ((ch - 48) > MAXGAMES || (ch - 48) < 0));

	if(ch == 'n'){
		role = ACTIVE_PLAYER;
		if(strlen(playername)==0) strcpy(playername,"ANON");
	}else{ //SPECTATOR
		role = SPECTATOR;																							//??
		//set choosen port																										// really necessary?
		port=tmp_game_mem[ch - 48].port;																								//game hardcoded!!
		strcpy(playername,""); //send empty playername
	}
	endwin();

	//Send playername to server
	ret = send(gamesocket, playername, PLAYER_NAME_LEN + 1, 0);
	if(ret < 0) error_handler(ERR_SEND);

	uint16_t buf = 0;

	switch(role){
		case ACTIVE_PLAYER:
			//recive new gameport
			msgSize = recv(gamesocket, &buf , sizeof(uint16_t), 0);
			printf("got Port: %d\n", buf);
			close(gamesocket);

			//Connect
			gamesocket = connect2server(ip, buf);
			if(gamesocket < 0) error_handler(gamesocket);

			//final handshake
			buf = 0;
			msgSize = recv(gamesocket, &buf , sizeof(uint16_t), 0);
			printf("got: %d\n", buf);
			//enter gameloop
			gameloop(gamesocket);
			break;
		default:
			close(gamesocket);
			//conect to game as spectator
			gamesocket = connect2server(ip, port);
			if(gamesocket < 0) error_handler(gamesocket);

			spectate(gamesocket, tmp_game_mem[ch - 48].name);
			break;
	}

	return 0;
}

void gameloop(int gamesocket){
	int ret = 0;
	int msgSize = -1;
	int ch;



  // GAME STARTS HERE ------------------------------------------------
	  client_data_exchange_container = malloc(SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
	  //memset(client_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
	  init_graphix();
	  print_scorescr(playername, c_player.score, c_player.life, 0);		// TODO: change from 0 to number of spectators!
	  print_statscr();
	  usleep(DELAY);

  //BEGIN MAIN LOOP-------------------------------------------------------------
	while(1) {
	//clientside -> start

		//GET TCP PACKAGE
		//memset(client_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
		msgSize = recv(gamesocket, client_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER, 0);

		if(msgSize <= 0){
			if(errno != EWOULDBLOCK)error_handler(ERR_RECV);
		}

		//Look if player is game over
		if(((Player*)client_data_exchange_container)->life == 0){
			ret = disp_infoscr('g');
			if(ret == 'q'){					//really exit
				c_player.instructions |= QUIT;
				ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
					if(ret < 0) error_handler(ERR_SEND);
				break;
			}
			else if(ret == 'r'){			//start new game
				c_player.instructions |= RESTART;
				ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
					if(ret < 0) error_handler(ERR_SEND);
				free(client_data_exchange_container);
				gameloop(gamesocket);
				return;
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
		print_scorescr(playername, c_player.score, c_player.life, 0);		// TODO: change from 0 to number of spectators!

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
					if(ret < 0) error_handler(ERR_SEND);
				free(client_data_exchange_container);
				gameloop(gamesocket);
				break;
			}
		}
		else if(ch == 'p') disp_infoscr('p');		//game paused
		else if(ch == 'r'){						//restart game
			ret = disp_infoscr('r');
			if(ret == 'y'){					//really restart
				c_player.instructions |= RESTART;
				ret = send(gamesocket, &(c_player.instructions), sizeof(char), 0);
					if(ret < 0) error_handler(ERR_SEND);
				free(client_data_exchange_container);
				gameloop(gamesocket);
				return;
			}
		}

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
	uint8_t tmp_byte = 0;

	  // GAME STARTS HERE ------------------------------------------------
		  client_data_exchange_container = malloc(SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
		  //memset(client_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
		  init_graphix();
		  print_scorescr(playername, c_player.score, c_player.life, 0);		// TODO: change from 0 to number of spectators!
		  usleep(DELAY);

			//request first packet
			tmp_byte = ACK;
			send(socket, &tmp_byte, sizeof(tmp_byte), MSG_DONTWAIT);

	  //BEGIN MAIN LOOP-------------------------------------------------------------
		while(1) {
		//clientside -> start


			ch = wgetch(fieldscr);


			if(ch == 'q'){						//quit game
				ret = disp_infoscr(ch);
				if(ret == 'y'){
				 	tmp_byte = ENDOFCON;
					send(socket, &tmp_byte, sizeof(tmp_byte), 0);				//really exit
					close(socket);
					endwin();
					exit(EXIT_SUCCESS);
				}else {

				}		//TODO!! RESTORE SCREEN DUMP!!
			}

			//acknowloedge that we are ready to receive!
			//GET TCP PACKAGE
			//memset(client_data_exchange_container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
			msgSize = recv(socket, client_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER, 0);
			tmp_byte = ACK;
			send(socket, &tmp_byte, sizeof(tmp_byte), MSG_DONTWAIT);

			if(msgSize <= 0){
				if(errno != EWOULDBLOCK){
					error_handler(-8);
				}else{
					usleep(500000);
					continue;
				}
			}

			//Look if player is game over
			if(((Player*)client_data_exchange_container)->life == 0){
					c_player.instructions |= QUIT;
					close(socket);
					endwin();
					exit(EXIT_SUCCESS);
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
			print_scorescr(playername, c_player.score, c_player.life, 0);		// TODO: change from 0 to number of spectators!
			usleep(DELAY);


		}

	  beep();
	  free(client_data_exchange_container);
	  endwin();

		// Disconnect from server
		ret = close(socket);
		if(ret < 0) error_handler(-29);

		return;

}
