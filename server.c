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
#include <stdio.h>
#include "communication.h"
#include "graphX.h"

//server-variables
Player s_player = {{MX/2, MY-2}, 0, 5, 0, 1, 0};
Object s_obj[MX * MY] = {{{0,0}, 0, 0, UPDATED}};
char *server_data_exchange_container = NULL;
char *server_res_buf = 0;
Shot s_shots[AMUNITION] = { {{0, 0}, 0} };
char dir = 'r';		//direction objects move to

time_t t;
time_t currentTime;

//serverside functions
int launch_gameserver(int port);
void gameloop(int socket);
void shoot(Shot _shots[AMUNITION] ,uint16_t init_pos[2], Object obj[MX * MY]);
int test_for_collision(uint16_t pos1[2], uint16_t pos2[2], int8_t planned_step_x, int8_t planned_step_y);
int test_for_collision_with_object(uint16_t pos1[2], Object obj[MX * MY], int8_t planned_step_x, int8_t planned_step_y);
int update_player(Player *_player, Object obj[MX * MY], uint16_t max_x, uint16_t max_y);
void place_object(int lines, int appearChance);	//if lines == 0: object will appear at random xy-Position
int move_object(uint8_t type);
int get_empty_obj_num(int objn);
uint16_t which_port(int socket);

int max_y = 0, max_x = 0;

int main(int argc, char **argv) {
	int ret = 0;
	int gamesocket, new_client, new_socket;
	int port = STD_PORT;
	struct sockaddr_in address;
	socklen_t addrLength = sizeof(address);
	int pid = 0;

	// Check arguments
	if(argc > 2){
		if(strcmp(argv[1], "-p") == 0) port = atoi(argv[2]);
		else error_handler(-1);
	}
	if(port <= PORT_MIN || port >= PORT_MAX) error_handler(-2);

	// Launch gameserver
	gamesocket = launch_gameserver(port);
	if(gamesocket < 0) error_handler(gamesocket);

	// Get connections
	while(1){
		new_client = accept(gamesocket, (struct sockaddr *) NULL, NULL);
		if(new_client < 0) error_handler(-6);

		//assuming player wants to start a new game and making a server for him
		//creating new socket bound to any available port
		new_socket = launch_gameserver(NEXT_AVAILABLE);
		if(new_socket < 0) error_handler(new_socket);
		//looking up the port the socket was bound to and sending it to the client.
		uint16_t tmp_port = which_port(new_socket);
		ret = send(new_client, &tmp_port, sizeof(uint16_t), 0);
		//closing the connection (socket still open)
		close(new_client);
		//socket can now be given to child!


		// Create child process
		pid = fork();
		if(pid < 0) error_handler(-11);

		if (pid == 0){
			close(gamesocket);
			gameloop(new_socket);
		}
		else close(new_client);
	}

//TODO: EXIT STRATEGY
	// Disconnect from client
	ret = close(gamesocket);
	if(ret < 0) error_handler(-9);

	return 0;
}

void gameloop(int socket){
	int ret = 0;
	int msgSize = -1;
	int loopCount = 0;					//count number of while-circles
	int appearTime = 20;				//number of while-circles until new objects appear
	int appearChance = 20;				//chance that an object appears at a position
	char playername[PLAYER_NAME_LEN + 1] = "";

	//waiting for client to connect!
	int client = accept(socket, (struct sockaddr *) NULL, NULL);
	if(client < 0) error_handler(-6);
	//final handshake
	uint16_t tmp_int = 22;
	ret = send(client, &tmp_int, sizeof(uint16_t), 0);



	server_data_exchange_container = NULL;
	server_res_buf = 0;
	dir = 'r';

	s_player.pos[0] = MX/2;
	s_player.pos[1] = MY-2;
	s_player.modifier = 0;
	s_player.life = 3;
	s_player.score = 0;
	s_player.amunition = 1;
	s_player.instructions = 0;

	s_shots[0].pos[0] = 0;
	s_shots[0].pos[1] = 0;
	s_shots[0].active = 0;

	for(int i = 0; i < MX*MY; i++){
		s_obj[i].pos[0] = 0;
		s_obj[i].pos[1] = 0;
		s_obj[i].type = 0;
		s_obj[i].life = 0;
		//Art* art;
		s_obj[i].status = UPDATED;
	}

	//get playername from client
	msgSize = recv(client, playername, PLAYER_NAME_LEN + 1, 0);
	if(msgSize <= 0) error_handler(-8);

  //SERVERSIDE INIT
	init_graphix();
	print_scorescr(playername, s_player.score, s_player.life, 0);		// TODO: change from 0 to number of spectators!

	//add attributes
	server_data_exchange_container = malloc(SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);

	//create objects on random positions
	srand((unsigned) time(&t));
	/*for(int i = 0; i < 15; i++){
		place_object(0, 60);
	}*/

	//create some objects in lines
	place_object(3, appearChance);

  //BEGIN MAIN LOOP-------------------------------------------------------------
	while(1) {
		time(&currentTime);
		//usleep(DELAY);
		//encode TCP package
		handle_package(server_data_exchange_container, &s_player, s_obj, s_shots, ASSEMBLE);

		//transmit TCP package
	//TODO: Calculate size of container to send
		ret = send(client, server_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER, 0);
		if(ret < 0){
			free(server_data_exchange_container);
			error_handler(-7);
		}


		//get TCP package
		msgSize = recv(client, &(s_player.instructions), sizeof(s_player.instructions), 0);
		if(msgSize <= 0){
			free(server_data_exchange_container);
			error_handler(-8);
		}

		//check for quit, restart
		if(s_player.instructions & QUIT) break;
		else if(s_player.instructions & RESTART){
			free(server_data_exchange_container);
			endwin();
			printf("GAME RESTARTET BY PLAYER\n");
			gameloop(socket);
			return;
		}

		//initiate shot & update player
		if(s_player.instructions & INIT_SHOT)shoot(s_shots, s_player.pos, s_obj);
		ret = update_player(&s_player, s_obj, MX, MY);
		if(ret == 1) s_player.life--;


		//update shots
		shoot(s_shots, NULL, s_obj);

		//move objects
		if(loopCount == appearTime){
			ret = move_object(1);
			if(ret == 1) s_player.life--;			//set player-lifes to 0 --> let player know game over
			loopCount = 0;
		}

		//make falling objects faster than sideways moving ones!
		if(loopCount%(appearTime/8) == (appearTime/8)-1){
			ret = move_object(2);
			if(ret == 1) s_player.life--;
		}

		draw_player(&s_player, 'o');
		draw_obj(s_obj, 'X');
		draw_shot(s_shots, '|');
		//simple frame-change indicator
		frame_change();

		wrefresh(fieldscr);

		draw_player(&s_player, ' ');
		draw_obj(s_obj, ' ');
		draw_shot(s_shots, ' ');
		print_scorescr(playername, s_player.score, s_player.life, 0);		// TODO: change from 0 to number of spectators!

		loopCount++;
	}
	beep();
	free(server_data_exchange_container);
	endwin();
	printf("QUIT BY PLAYER\n");
	exit(EXIT_SUCCESS);
}

void shoot(Shot _shots[AMUNITION] ,uint16_t init_pos[2], Object obj[MX * MY]){
  for(int i = 0; i < AMUNITION; i++){
    if(!_shots[i].active && init_pos != NULL){
      _shots[i].active = 1;
      _shots[i].pos[0] = init_pos[0];
      _shots[i].pos[1] = init_pos[1];
    }
	else if(_shots[i].active && obj != NULL){
		if((_shots[i].pos[1] < 2) || test_for_collision_with_object(_shots[i].pos, obj, 0, -1)){
			for(int o = 0; o < MX * MY; o++){
				if((obj[o].life > 0) &&(obj[o].pos[0] ==  _shots[i].pos[0]) && (obj[o].pos[1] ==  _shots[i].pos[1] - 1)){
					obj[o].life--;
					if(obj[o].life == 0) s_player.score += 50;
					obj[o].status = UPDATED;
				}
				_shots[i].active = 0;
			}
		}
	_shots[i].pos[1] -= 1;
    }
  }

}

int test_for_collision(uint16_t pos1[2], uint16_t pos2[2], int8_t planned_step_x, int8_t planned_step_y){
  if(!(pos1[0] + planned_step_x == pos2[0] && pos1[1] + planned_step_y == pos2[1])) return 0;
  return 1;
}

int test_for_collision_with_object(uint16_t pos1[2], Object obj[MX * MY], int8_t planned_step_x, int8_t planned_step_y){
  int collision = 0;
  for(int i = 0; i < (MX * MY); i++){
      if(test_for_collision(pos1, obj[i].pos, planned_step_x, planned_step_y) && (obj[i].life > 0)) collision ++;
      if(collision) return 1;
  }

  return collision;
}

int update_player(Player *_player,Object obj[MX * MY], uint16_t max_x, uint16_t max_y){
  //update player position
	int gameover = 0;

  if ((_player->pos[0] < (max_x - 1))&&(_player->instructions & RIGHT)) {
    if(!test_for_collision_with_object(_player->pos, obj, 1, 0)) _player->pos[0]++;
		else gameover = 1;
  } else if ((_player->pos[0]  > 0)&&(_player->instructions & LEFT)) {
    if(!test_for_collision_with_object(_player->pos, obj, -1, 0)) _player->pos[0]--;
		else gameover = 1;
  } else if ((_player->pos[1] < (max_y - 1))&&(_player->instructions & DOWN)) {
    if(!test_for_collision_with_object(_player->pos, obj, 0, 1)) _player->pos[1]++;
		else gameover = 1;
  } else if((_player->pos[1]  > MY - HEIGHT_OF_PLAYER_SPACE)&&(_player->instructions & UP)) {
    if(!test_for_collision_with_object(_player->pos, obj, 0, -1)) _player->pos[1]--;
		else gameover = 1;
  }
  return gameover;
}

//GET FUNCTION TO EXTERNAL FILE
void handle_package(char *container, Player *player, Object obj[MX * MY], Shot shots[AMUNITION], int mode){
  if(mode == ASSEMBLE){

    memset(container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
    char *tmp = container;
    memcpy(tmp, &s_player, sizeof(Player));
    tmp += sizeof(Player);
    memcpy(tmp, shots, sizeof(Shot) * AMUNITION);
    tmp += sizeof(Shot) * AMUNITION;
    uint16_t tmp_int = 0;
    for(uint16_t i = 0; i < MX * MY; i++){
      if(obj[i].status & UPDATED){
				//printf("updated\n");
        memcpy(tmp + sizeof(uint16_t) + (sizeof(Object) + sizeof(uint16_t)) * tmp_int, &i, sizeof(uint16_t));
        memcpy(tmp + sizeof(uint16_t) + sizeof(Object) * tmp_int + sizeof(uint16_t) * (tmp_int + 1), &(obj[i]), sizeof(Object));
        tmp_int++;
        obj[i].status = NO_CHANGE;
      }
    }
    memcpy(tmp, &tmp_int, sizeof(uint16_t));
    tmp = NULL;
	}
  }


//-------------------------------------------------

int move_object(uint8_t type){
	int yOffset = 0;
	int xOffset = 0;
	int appearChance = 20;
	int gameover = 0;
	int max_y = 0;

	if(type == 1){
		//set x-move direction
		if(dir == 'r') xOffset = 1;		//move right
		else xOffset = -1;				//move left

		//check if y-move is necessary
		for(int i = 0; i < (MX*MY); i++){
			if(s_obj[i].type == 1 && s_obj[i].life > 0){		//check if it is a wandering object still alive
				if((dir == 'r' && s_obj[i].pos[0]+1 >= MX) || (dir == 'l' && s_obj[i].pos[0] <= 0)){		//if wandering right/left would hit wall
					yOffset = 1;
					xOffset = 0;		//no x-move when y-move
					break;
				}
			}
		}

		//wander right or left
		for(int i = 0; i < (MX*MY); i++){
			if(s_obj[i].type == 1 && s_obj[i].life > 0){	//check if it is a wandering object still alive
				s_obj[i].pos[0] = s_obj[i].pos[0] + xOffset;
				s_obj[i].pos[1] = s_obj[i].pos[1] + yOffset;
				s_obj[i].status = UPDATED;
				if(s_obj[i].pos[1] > MY - HEIGHT_OF_PLAYER_SPACE - 2){		//check if a y-move would hit lower border
					gameover = 1;
					break;
				}

				if(s_obj[i].pos[1] > max_y) max_y = s_obj[i].pos[1];
			}
		}

		//Randomly spawn an object 2
		int shoot = rand() % 100;
		int currMaxY = 0;
		int currMinX = MX;
		if(shoot > 70){
			int o = get_empty_obj_num(0);

			for(int i = 0; i < (MX*MY); i++){
				if(abs(s_player.pos[0]-s_obj[i].pos[0]) < currMinX){
					currMinX = s_player.pos[0]-s_obj[i].pos[0];
					if(s_obj[i].pos[1] > currMaxY) currMaxY = s_obj[i].pos[1];
				}
			}

			s_obj[o].pos[0] = s_player.pos[0] - currMinX;
			s_obj[o].pos[1] = currMaxY + 1;
			s_obj[o].life = 1;
			s_obj[o].type = 2;
			s_obj[o].status = UPDATED;

		}


		//make new line of objects at top
		if(yOffset == 1){
			place_object(1, appearChance);
			if(dir == 'r') dir = 'l';		//change direction for next time
			else dir = 'r';
		}
	}else if(type == 2){

		for(int i = 0; i < (MX*MY); i++){
			if(s_obj[i].type == 2 && s_obj[i].life > 0){		//check if it is a falling object and still alive
				if(test_for_collision(s_obj[i].pos, s_player.pos, 0, 1)){
					s_obj[i].life = 0;
					gameover = 1;
				}else if(s_obj[i].pos[1] < MY ){
					s_obj[i].pos[1]++;
				}else {
					s_obj[i].life = 0;
				}
				s_obj[i].status = UPDATED;
			}
		}

	}
	return gameover;
}

void place_object(int lines, int appearChance){
	int objn = 0;

	if(lines == 0 && (rand() % 100) < appearChance){
		objn = get_empty_obj_num(objn);
		s_obj[objn].pos[0] = rand() % MX;
		s_obj[objn].pos[1] = rand() % MY;
		//choose how hard it should be!
		//s_obj[objn].life = (rand() % 3) +1;
		s_obj[objn].life = 1;
		s_obj[objn].type = 1;

		s_obj[objn].status = UPDATED;
	}

	for(int i = 0; i < lines; i++){					//do it for given number of lines
		for(int j = OBJ_LINE_L_OFFSET; j < (MX - OBJ_LINE_R_OFFSET); j++){			//do it from 3rd position in row to 3rd-last position in row
			if((rand() % 100) < appearChance){
				objn = get_empty_obj_num(objn);
				s_obj[objn].pos[0] = j;
				s_obj[objn].pos[1] = i;
				s_obj[objn].type = 1;				//identify wandering objects
				//choose how hard it should be!
		//s_obj[objn].life = (rand() % 3) +1;
				s_obj[objn].life = 1;
				s_obj[objn].status = UPDATED;
				objn++;
			}
		}
	}
}

int get_empty_obj_num(int objn){
	while(s_obj[objn].life > 0){		//search for dead objects
		objn++;
	}
	return objn;
}

int launch_gameserver(int port){
	int ret = 0;
	int gamesocket = 0;
	struct sockaddr_in address;

	// Fill in connection information
	address.sin_family = AF_INET;			//IPv4 protocol
	address.sin_addr.s_addr = INADDR_ANY; 	//Receive packets from any address
	if(port != NEXT_AVAILABLE)address.sin_port = htons(port);			//Port number htons converts byte order
	else address.sin_port = 0;
	// Create Socket		Address family: AF_INET: IPv4; Socket type: SOCK_STREAM: Stream; Protocol: 0: Standard to socket type
	gamesocket = socket(AF_INET, SOCK_STREAM, 0);
	if (gamesocket < 0) return -3;

	// Bind Socket to process
	ret = bind(gamesocket, (struct sockaddr*)&address, sizeof(address));
	if(ret < 0) return -4;

	// Make listener (queue) for new connections
	ret = listen(gamesocket, NUM_CONNECTIONS);
	if(ret < 0) return -5;

	return gamesocket;
}

uint16_t which_port(int socket){
	struct sockaddr_in address;
	socklen_t size = sizeof(address);
	getsockname(socket, (struct sockaddr*)&address, &size);

	return ntohs(address.sin_port);
}
