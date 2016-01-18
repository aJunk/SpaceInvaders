/**************************************************************************
 * SPACEINVADERS GAME - client (header)
 * Headerfile for a TCP/IP based Spaceinvaders game for Linux and MacOS.
 * Provides several prototypes of functions for the server.
 *
 * written by Philipp Gotzmann, Alexander Junk and Johannes Rauer
 * UAS Technikum Wien, BMR3_2014
 */

#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

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
#include "server.h"
#include "communication.h"
#include "graphX.h"

int launch_gameserver(int port);			//makes a socket, binds it and listens on given port (if NEXT_AVAILABLE is given it takes next available port); returns socket-fd
void gameloop(int socket[],char playername[]);		//loop where game is executed, send/recv to player takes place
int check_alive (Game game_mem[]);		//checks if children are still alive and updates memory; returns nuber of activ children
void shoot(Shot _shots[AMUNITION] ,uint16_t init_pos[2], Object obj[MX * MY]);
int test_for_collision(uint16_t pos1[2], uint16_t pos2[2], int8_t planned_step_x, int8_t planned_step_y);
int test_for_collision_with_object(uint16_t pos1[2], Object obj[MX * MY], int8_t planned_step_x, int8_t planned_step_y);
int update_player(Player *_player, Object obj[MX * MY], uint16_t max_x, uint16_t max_y);
void place_object(int lines, int appearChance);		//places objects in lines on fieldscreen with a given appear chance, if lines == 0: object will appear at random position
int move_object(uint8_t type);					//moves the objects with given type 1 left or right/type 2 objects shoot; returns if player is gameover (line hits player-space)
int get_empty_obj_num(int objn);				//searches for a free space in obj-array, starting at a given objectnummer; returns int to next free space
uint16_t which_port(int socket);				//looks up to which port an existing socket is bound; returns port
void sig_handler();

#endif
