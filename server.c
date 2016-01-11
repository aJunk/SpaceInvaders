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

//server-variables
Player s_player = {{0,0}, 0, 5, 1, 0};
Object s_obj[MX * MY] = {{{0,0}, 0, 0, 0}};
char *server_data_exchange_container = NULL;
char *server_res_buf = 0;
Shot s_shots[AMUNITION] = { {{0, 0}, 0} };
char dir = 'r';		//direction objects move to

time_t t;

//serverside functions
int launch_gameserver(int port);
void gameloop(int gamesocket);
void shoot(Shot _shots[AMUNITION] ,uint16_t init_pos[2], Object obj[MX * MY]);
int test_for_collision(uint16_t pos1[2], uint16_t pos2[2], int8_t planned_step_x, int8_t planned_step_y);
int test_for_collision_with_object(uint16_t pos1[2], Object obj[MX * MY], int8_t planned_step_x, int8_t planned_step_y);
int update_player(Player *_player, Object obj[MX * MY], uint16_t max_x, uint16_t max_y);
void place_object(int lines, int appearChance);	//if lines == 0: object will appear at random xy-Position
void move_object(uint8_t type);
int get_empty_obj_num(int objn);

void screen_init();
void draw_obj(Object obj[MX * MY]);
void draw_player(Player *_player);
void draw_shot(Shot _shots[AMUNITION]);
void frame_change();

int max_y = 0, max_x = 0;

int main(int argc, char **argv) {
	int ret = 0;
	int gamesocket, new_gamesocket;
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
		new_gamesocket = accept(gamesocket, (struct sockaddr *) &address, &addrLength);
		if(new_gamesocket < 0) error_handler(-6);

		// Create child process			basic structure by http://www.tutorialspoint.com/unix_sockets/socket_server_example.htm
		pid = fork();
		if(pid < 0) error_handler(-11);

		if (pid == 0){
			close(gamesocket);
			gameloop(new_gamesocket);
		}
		else close(new_gamesocket);
	}

//TODO: EXIT STRATEGY
	// Disconnect from client
	ret = close(gamesocket);
	if(ret < 0) error_handler(-9);

	return 0;
}

void gameloop(int gamesocket){
	int ret = 0;
	int msgSize = -1;
	int loopCount = 0;					//count number of while-circles
	int appearTime = 25;				//number of while-circles until new objects appear
	int appearChance = 20;				//chance that an object appears at a position

//SERVERSIDE INIT
	screen_init();

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

		usleep(20000);
		//encode TCP package
		handle_package(server_data_exchange_container, &s_player, s_obj, s_shots, ASSEMBLE);

		//transmit TCP package
	//TODO: Calculate size of container to send
		ret = send(gamesocket, server_data_exchange_container, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER, 0);
		if(ret < 0){
			free(server_data_exchange_container);
			error_handler(-7);
		}

		//get TCP package
		msgSize = recv(gamesocket, &(s_player.instructions), sizeof(s_player.instructions), 0);
		if(msgSize <= 0){
			free(server_data_exchange_container);
			error_handler(-8);
		}

		//initiate shot & update player
		if(s_player.instructions & INIT_SHOT)shoot(s_shots, s_player.pos, s_obj);
		update_player(&s_player, s_obj, MX, MY);

		//update shots
		shoot(s_shots, NULL, s_obj);

		//move objects
		if (loopCount == appearTime){
			move_object(1);
			loopCount = 0;
		}

		//resizeterm(MY+2, MX+2);
		clear();
		wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');
		//draw player
		draw_player(&s_player);
		//draw all objects
		draw_obj(s_obj);
		//draw shots
		draw_shot(s_shots);
		//simple frame-change indicator
		frame_change();
		refresh();

		loopCount++;
	}
	beep();
	free(server_data_exchange_container);
	endwin();
	return;
}

void shoot(Shot _shots[AMUNITION] ,uint16_t init_pos[2], Object obj[MX * MY]){
  for(int i = 0; i < AMUNITION; i++){
    if(!_shots[i].active && init_pos != NULL){
      _shots[i].active = 1;
      _shots[i].pos[0] = init_pos[0];
      _shots[i].pos[1] = init_pos[1];
    }else if(_shots[i].active && obj != NULL){

      if((_shots[i].pos[1] < 2)||test_for_collision_with_object(_shots[i].pos, obj, 0, 0)){
          for(int o = 0; o < MX * MY; o++)if((obj[o].pos[0] ==  _shots[i].pos[0]) && (obj[o].pos[1] ==  _shots[i].pos[1])){
            obj[o].life--;
            obj[o].status = UPDATED;
          }
        _shots[i].active = 0;
      }else{
        //mvprintw(_shots[i].pos[1], _shots[i].pos[0] + 1, "|");
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
  if ((_player->pos[0] < (max_x - 1))&&(_player->instructions & RIGHT)) {
    if(!test_for_collision_with_object(_player->pos, obj, 1, 0)) _player->pos[0]++;
  } else if ((_player->pos[0]  > 0)&&(_player->instructions & LEFT)) {
    if(!test_for_collision_with_object(_player->pos, obj, -1, 0)) _player->pos[0]--;
  } else if ((_player->pos[1] < (max_y - 1))&&(_player->instructions & DOWN)) {
    if(!test_for_collision_with_object(_player->pos, obj, 0, 1)) _player->pos[1]++;
  } else if((_player->pos[1]  > 0)&&(_player->instructions & UP)) {
    if(!test_for_collision_with_object(_player->pos, obj, 0, -1)) _player->pos[1]--;
  }
  return 0;
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
void screen_init(){
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
	curs_set(FALSE);
	timeout(0);
	resizeterm(MY+2, MX+2);
	getmaxyx(stdscr, max_y, max_x);
	clear();

	refresh();
	wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');
	// Global var `stdscr` is created by the call to `initscr()`

	//init colors
	start_color();			/* Start color 			*/
	init_pair(obj_colour, COLOR_RED, COLOR_BLACK);
	init_pair(bkg_colour, COLOR_GREEN, COLOR_BLACK);
	init_pair(player_colour, COLOR_YELLOW, COLOR_BLACK);
}

void draw_obj(Object _obj[MX * MY]){
  attron(COLOR_PAIR(obj_colour));

  for(int i = 0; i < MX * MY; i++)if(_obj[i].life > 0)mvprintw(_obj[i].pos[1] + 1, _obj[i].pos[0] + 1, "X");

  attron(COLOR_PAIR(bkg_colour));
}

void draw_player(Player *_player){
  attron(COLOR_PAIR(player_colour));

  mvprintw(_player->pos[1] + 1, _player->pos[0] + 1, "o");

  attron(COLOR_PAIR(bkg_colour));
}

void draw_shot(Shot _shots[AMUNITION]){
  if(_shots[0].active)mvprintw(_shots[0].pos[1], _shots[0].pos[0] + 1, "|");
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

void move_object(uint8_t type){
	int yOffset = 0;
	int xOffset = 1;
	int appearChance = 20;

	if(type == 1){
		//check if y-move is necessary
		for(int i = 0; i < (MX*MY); i++){
			if(s_obj[i].type == 1 && s_obj[i].life > 0){		//check if it is a wandering object
				if(dir == 'r' && s_obj[i].pos[0]+1 >= MX){		//if wandering right would hit wall
					yOffset = 1;
					xOffset = 0;
					break;
				}
				else if(dir == 'l' && s_obj[i].pos[0] <= 0){	//if wandering left would hit wall
					yOffset = 1;
					xOffset = 0;
					break;
				}
			}
		}

		//wander right or left
		if(dir == 'r'){
			for(int i = 0; i < (MX*MY); i++){
				if(s_obj[i].type == 1 && s_obj[i].life > 0){	//check if it is a wandering object
					s_obj[i].pos[0] = s_obj[i].pos[0] + xOffset;
					s_obj[i].pos[1] = s_obj[i].pos[1] + yOffset;
					s_obj[i].status = UPDATED;
				}
			}
		}
		else if(dir == 'l'){
			for(int i = 0; i < (MX*MY); i++){
				if(s_obj[i].type == 1 && s_obj[i].life > 0){	//check if it is a wandering object
					s_obj[i].pos[0] = s_obj[i].pos[0] - xOffset;
					s_obj[i].pos[1] = s_obj[i].pos[1] + yOffset;
					s_obj[i].status = UPDATED;
				}
			}
		}

		if(yOffset == 1){
			place_object(1, appearChance);
			if(dir == 'l'){
				dir = 'r';		//change direction for next time
			}
			else{
				dir = 'l';
			}
		}
	}
}

void place_object(int lines, int appearChance){
	int objn = 0;

	if(lines == 0 && (rand() % 100) < appearChance){
		objn = get_empty_obj_num(objn);
		s_obj[objn].pos[0] = rand() % MX;
		s_obj[objn].pos[1] = rand() % MY;
		s_obj[objn].life = (rand() % 3) +1;
		s_obj[objn].status = UPDATED;
	}

	for(int i = 0; i < lines; i++){					//do it for given number of lines
		for(int j = 3; j < (MX-3); j++){			//do it from 3rd position in row to 3rd-last position in row
			if((rand() % 100) < appearChance){
				objn = get_empty_obj_num(objn);
				s_obj[objn].pos[0] = j;
				s_obj[objn].pos[1] = i;
				s_obj[objn].type = 1;				//identify wandering objects
				s_obj[objn].life = (rand() % 3) + 1;
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
	address.sin_port = htons(port);			//Port number htons converts byte order

	// Create Socket		Address family: AF_INET: IPv4; Socket type: SOCK_STREAM: Stream; Protocol: 0: Standard to socket type
	gamesocket = socket (AF_INET, SOCK_STREAM, 0);
	if (gamesocket < 0) return -3;

	// Bind Socket to process
	ret = bind(gamesocket, (struct sockaddr*)&address, sizeof(address));
	if(ret < 0) return -4;

	// Make listener (queue) for new connections
	ret = listen(gamesocket, NUM_CONNECTIONS);
	if(ret < 0) return -5;

	return gamesocket;
}
