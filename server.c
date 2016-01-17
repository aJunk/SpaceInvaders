#define _POSIX_C_SOURCE 199309L
#define _BSD_SOURCE
#include <unistd.h>

#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>
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
int max_y = 0, max_x = 0;

//serverside functions
int launch_gameserver(int port);					//makes a socket, binds it and listens on given port (if NEXT_AVAILABLE is given it takes next available port); returns socket-fd
void gameloop(int socket[],char playername[]);		//loop where game is executed, send/recv to player takes place
int check_alive (Game game_mem[]);
void shoot(Shot _shots[AMUNITION] ,uint16_t init_pos[2], Object obj[MX * MY]);
int test_for_collision(uint16_t pos1[2], uint16_t pos2[2], int8_t planned_step_x, int8_t planned_step_y);
int test_for_collision_with_object(uint16_t pos1[2], Object obj[MX * MY], int8_t planned_step_x, int8_t planned_step_y);
int update_player(Player *_player, Object obj[MX * MY], uint16_t max_x, uint16_t max_y);
void place_object(int lines, int appearChance);		//places objects in lines on fieldscreen with a given appear chance, if lines == 0: object will appear at random position
int move_object(uint8_t type);						//moves the objects with given type 1 left or right/type 2 objects shoot; returns if player is gameover (line hits player-space)
int get_empty_obj_num(int objn);					//searches for a free space in obj-array, starting at a given objectnummer; returns int to next free space
uint16_t which_port(int socket);

void sig_handler(){									//if a user/system interrupts
	printf("*** Server ended due to interrupt ***\n");		//printf may be interrupted but better than don't handling case
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv){
	int ret = 0;
	int gamesocket, new_client, new_socket[2]; //new_socket[0] = gamesocket (blocking); new_socket[1] =spectatorsocket (nonblocking)
	int port = STD_PORT;
	Game game_mem[MAXGAMES]={{{""},0,0,0}};
	int cpid = 0;
	char playername[PLAYER_NAME_LEN+1]="";
	int numgames = 0;
	int msgSize;
	int i;

	struct sigaction sig = {.sa_handler = sig_handler};
	sigemptyset(&sig.sa_mask);
	sigaction(SIGINT, &sig, NULL);
	sigaction(SIGPIPE, &sig, NULL);
  
	// Check arguments
	if(argc > 2){
		if(strcmp(argv[1], "-p") == 0) port = atoi(argv[2]);
		else error_handler(ERR_S_ARG);
	}
	if(port <= PORT_MIN || port >= PORT_MAX) error_handler(ERR_INVALID_PORT);

	// Launch gameserver
	gamesocket = launch_gameserver(port);
	if(gamesocket < 0) error_handler(gamesocket);
	print_server_msg(0, SUCCESS, "Gameserver launched. Port: ", port, "");

	// Get connections
	while(1){
		new_client = accept(gamesocket, (struct sockaddr *) NULL, NULL);

		if(new_client < 0) print_server_msg(0, ERROR, "Accept failed", 0, "");		//error_handler(ERR_CONNECT);
		else print_server_msg(0, SUCCESS, "Client connected", 0, "");

		numgames = check_alive(game_mem);
		ret = send(new_client, &game_mem, sizeof(Game) * MAXGAMES, 0);

		//get playername from client
		msgSize = recv(new_client, playername, PLAYER_NAME_LEN + 1, 0);
		if(msgSize <= 0) print_server_msg(0, ERROR, "Receiving playername failed", 0, "");		//error_handler(ERR_RECV)


		if(strlen(playername) != 0){ //player wants to start a new game
			//check if maximal number of games is already reached

			print_server_msg(0, INFO, "Client wants to play. Playername: ", 0, playername);

			numgames = check_alive(game_mem);
			if(numgames >= MAXGAMES) print_server_msg(0, ERROR, "Receiving playername failed", 0, ""); 		//error_handler(ERR_MAX_GAMES);	//TODO: HANDLE BETTER

			//creating new gamesocket bound to any available port
			new_socket[0] = launch_gameserver(NEXT_AVAILABLE);
			if(new_socket[0] < 0) print_server_msg(0, ERROR, "Launching gameserver failed. Returned: ", new_socket[0], "");			//error_handler(new_socket);
			//creating new spectatorsocket bound to any available port
			new_socket[1] = launch_gameserver(NEXT_AVAILABLE);
			if(new_socket[1] < 0) print_server_msg(0, ERROR, "Creating spectatorsocket failed. Returned: ", new_socket[1], "");			//error_handler(new_socket);
			//cahange spectatorsocket to nonblocking mode
			ret = fcntl(new_socket[1], F_SETFL, fcntl(new_socket[1], F_GETFL, 0) | O_NONBLOCK);
			if (ret == -1) perror("calling fcntl");																									//TODO: Errorhandler
			//looking up the port the gamesocket was bound to and sending it to the client.
			uint16_t tmp_port = which_port(new_socket[0]);

			//send port
			ret = send(new_client, &tmp_port, sizeof(uint16_t), 0);
			//closing the connection (socket still open)
			close(new_client);
			//socket can now be given to child!

			print_server_msg(0, INFO, "New gameserver launched, port transmitted. Port: ", tmp_port, "");
			//looking up the port the spectatorsocket was bound
			tmp_port = which_port(new_socket[1]);
			print_server_msg(0, INFO, "Socket for spectators created. Port: ", tmp_port, "");

			// Create child process
			cpid = fork();
			if(cpid < 0) print_server_msg(0, ERROR, "Forking failed", 0, "");			//error_handler(ERR_FORK);

			if (cpid == 0){
				close(gamesocket);
				pid_t pid = getpid();
				print_server_msg(pid, INFO, "Temp socket disconnected", 0, "");
				gameloop(new_socket,playername);
			}
			else{
				//go to next empty place
				for(i=0; (i<MAXGAMES) && (game_mem[i].pid!=0); i++);

				strcpy(game_mem[i].name,playername);
				game_mem[i].pid = cpid;
				game_mem[i].port = tmp_port;
				close(new_socket[0]);
				close(new_socket[1]);
			}
		}
		else{ //wants to become a spectator

		}
		close(new_client);
	}
	return 0;
}

void gameloop(int socket[], char playername[]){
	int ret = 0;
	int msgSize = -1;
	int temp;
	int i;
	int loopCount = 0;					//count number of while-circles
	int appearTime = 20;				//number of while-circles until new objects appear
	int appearChance = 20;				//chance that an object appears at a position
	static uint8_t recursive = 0;		//check if game is restarted
	static int client = -1;
	int spectator[MAXSPECT]={0};
	int numspect = 0;
	pid_t pid = getpid();

	print_server_msg(pid, SUCCESS, "Gameloop entered. Playername:", 0, playername);

	//waiting for client to connect!
	if(!recursive){
		client = accept(socket[0], (struct sockaddr *) NULL, NULL);
		//final handshake
		uint16_t tmp_int = 22;
		ret = send(client, &tmp_int, sizeof(uint16_t), 0);
		print_server_msg(pid, SUCCESS, "Client connected, handshake sent", 0, "");
	}
	if(client < 0) error_handler(ERR_CONNECT);
	recursive = 0;

  //SERVERSIDE INIT
	srand((unsigned) time(&t));
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

	for(int i = 0; i < AMUNITION; i++){
		s_shots[i].pos[0] = 0;
		s_shots[i].pos[1] = 0;
		s_shots[i].active = 0;
	}

	for(int i = 0; i < MX*MY; i++){
		s_obj[i].pos[0] = 0;
		s_obj[i].pos[1] = 0;
		s_obj[i].type = 0;
		s_obj[i].life = 0;
		//Art* art;
		s_obj[i].status = UPDATED;
	}

	//add attributes
	server_data_exchange_container = malloc(SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);

	//create some objects in lines
	place_object(3, appearChance);

  //BEGIN MAIN LOOP-------------------------------------------------------------
	while(1) {
		time(&currentTime);

		//encode TCP package
		handle_package(server_data_exchange_container, &s_player, s_obj, s_shots, ASSEMBLE);

		//transmit TCP package
  //TODO: Calculate size of container to send!!!!!!
		ret = send(client, server_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER, 0);
		if(ret <= 0){
			free(server_data_exchange_container);
			error_handler(ERR_SEND);
		}

		//HANDLE SPECTATORS
		if(numspect < MAXSPECT){
			temp = accept(socket[1], (struct sockaddr *) NULL, NULL);
			if(temp > 0){
				//go to next empty place
				for(i=0; (i < MAXSPECT) && (spectator[i] != 0); i++);
				spectator[i]=temp;
				numspect++;
				print_server_msg(pid, INFO, "New Spectator connected. Total spectators: ", numspect, "");
			}
		}
		for(i=0; i < MAXSPECT; i++){
			if(spectator[i] != 0){
					uint8_t tmp_byte = 0;
					ret = recv(spectator[i], &tmp_byte, sizeof(tmp_byte), MSG_DONTWAIT);
					if(ret == 0 || ((ret == 1) && (tmp_byte == ENDOFCON))){
						close(spectator[i]);
						spectator[i]= 0;
						numspect--;
						print_server_msg(pid, INFO, "Spectator disconected. Total spectators: ", numspect, "");
					}else if(tmp_byte == ACK){
						ret = send(spectator[i], server_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER, MSG_DONTWAIT);
					}else{
						//NOTHING TO DO HERE
						//ret = send(spectator[i], server_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER, MSG_DONTWAIT);
					}
			}
		}
		//------

		//get TCP package
		msgSize = recv(client, &(s_player.instructions), sizeof(s_player.instructions), 0);
		if(msgSize <= 0){
			free(server_data_exchange_container);
			error_handler(ERR_RECV);
		}

		//check for quit, restart
		if(s_player.instructions & QUIT) break;
		else if(s_player.instructions & RESTART){
			free(server_data_exchange_container);
			endwin();
			print_server_msg(pid, INFO, "Game restarted", 0, "");
			recursive = 1;
			gameloop(socket,playername);
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
			if(ret == 1) s_player.life--;			//set player-lifes to 0 to let player know game over
			loopCount = 0;
		}

		//make falling objects faster than sideways moving ones!
		if(loopCount%(appearTime/8) == (appearTime/8)-1){
			ret = move_object(2);
			if(ret == 1) s_player.life--;
		}

		//display in server log when player game over
		if(s_player.life == 0) print_server_msg(pid, INFO, "Player game over. Score/Name: ", s_player.score, playername);

		loopCount++;
	}
	beep();
	free(server_data_exchange_container);
	endwin();
	print_server_msg(pid, SUCCESS, "Game quit, ended normally. Score/name:", s_player.score, playername);
	close(client);
	for(int i = 0; i < MAXSPECT; i++)if(spectator[i] > 0)close(spectator[i]);
	close(socket[0]);
	shutdown(socket[1], SHUT_RDWR);
	close(socket[1]);
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
		//set x-move direction depending on given direction
		if(dir == 'r') xOffset = 1;		//move right
		else xOffset = -1;				//move left

		//check if y-move is necessary
		for(int i = 0; i < (MX*MY); i++){
			if(s_obj[i].type == 1 && s_obj[i].life > 0){		//check if it is a wandering object still alive
				if((dir == 'r' && s_obj[i].pos[0]+1 >= MX) || (dir == 'l' && s_obj[i].pos[0] <= 0)){		//if wandering right/left would hit wall
					yOffset = 1;
					xOffset = 0;								//no x-move when y-move
					break;
				}
			}
		}

		//wander right or left
		for(int i = 0; i < (MX*MY); i++){
			if(s_obj[i].type == 1 && s_obj[i].life > 0){		//check if it is a wandering object still alive
				s_obj[i].pos[0] = s_obj[i].pos[0] + xOffset;
				s_obj[i].pos[1] = s_obj[i].pos[1] + yOffset;
				s_obj[i].status = UPDATED;
				if(s_obj[i].pos[1] > MY - HEIGHT_OF_PLAYER_SPACE - 2){		//check if a y-move would hit lower border
					gameover = 1;
					break;
				}
				if(s_obj[i].pos[1] > max_y) max_y = s_obj[i].pos[1];		//get maximum y-position of objects
			}
		}

		//Randomly spawn an object type 2
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
	}
	else if(type == 2){
		for(int i = 0; i < (MX*MY); i++){
			if(s_obj[i].type == 2 && s_obj[i].life > 0){		//check if it is a falling object and still alive
				if(test_for_collision(s_obj[i].pos, s_player.pos, 0, 1)){
					s_obj[i].life = 0;
					gameover = 1;
				}
				else if(s_obj[i].pos[1] < (MY - 1) ){			//object isn't on bottom position yet
					s_obj[i].pos[1]++;
				}
				else {
					s_obj[i].life = 0;							//object hit bottom line
				}
				s_obj[i].status = UPDATED;
			}
		}
	}
	return gameover;
}

void place_object(int lines, int appearChance){
	int objn = 0;

	if(lines == 0 && (rand() % 100) < appearChance){		//place a object somewhere
		objn = get_empty_obj_num(objn);
		s_obj[objn].pos[0] = rand() % MX;
		s_obj[objn].pos[1] = rand() % MY;
		//choose how hard it should be!
		//s_obj[objn].life = (rand() % 3) +1;
		s_obj[objn].life = 1;
		s_obj[objn].type = 3;
		s_obj[objn].status = UPDATED;
	}

	for(int i = 0; i < lines; i++){						//do it for given number of lines
		for(int j = OBJ_LINE_L_OFFSET; j < (MX - OBJ_LINE_R_OFFSET); j++){		//do it from left to right offset
			if((rand() % 100) < appearChance){
				objn = get_empty_obj_num(objn);
				s_obj[objn].pos[0] = j;
				s_obj[objn].pos[1] = i;
				s_obj[objn].type = 1;					//identify wandering objects
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
	address.sin_family = AF_INET;									//IPv4 protocol
	address.sin_addr.s_addr = INADDR_ANY; 							//Receive packets from any address
	if(port != NEXT_AVAILABLE) address.sin_port = htons(port);		//if a port is given to function it is set
	else address.sin_port = 0;										//get next free port from system
	
	// Create Socket		Address family: AF_INET: IPv4; Socket type: SOCK_STREAM: Stream; Protocol: 0: Standard to socket type
	gamesocket = socket(AF_INET, SOCK_STREAM, 0);
	if (gamesocket < 0) return ERR_CREATE_SOCKET;

	// Bind Socket
	ret = bind(gamesocket, (struct sockaddr*)&address, sizeof(address));
	if(ret < 0) return ERR_BIND;

	// Make listener (queue) for new connections
	ret = listen(gamesocket, NUM_CONNECTIONS);
	if(ret < 0) return ERR_LISTENER;

	return gamesocket;
}

uint16_t which_port(int socket){
	struct sockaddr_in address;
	socklen_t size = sizeof(address);
	getsockname(socket, (struct sockaddr*)&address, &size);

	return ntohs(address.sin_port);
}

int check_alive (Game game_mem[]){

  int status;
  int numgames=0;
  pid_t result;

  for(int i=0; i<MAXGAMES; i++){
      if(game_mem[i].pid != 0){
          result = waitpid(game_mem[i].pid, &status, WNOHANG); //return imediately if process is still alive (Option WNOHANG)
          if (result == 0) {
                numgames ++;
          }
          else if (result == -1) {
                perror("waitpid");  //Error -> Exit?
          }
          else {
                game_mem[i].pid = 0;      // Child exited -> delete entry
								game_mem[i].port = 0;
								strcpy(game_mem[i].name, "");
          }
      }
  }
  return numgames;
}
//todo: errorhandler
