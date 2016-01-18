#include "communication.h"
#include "graphX.h"

#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

int connect2server(char ip[16], int port);			//creates a socket and connects to server with given ip and port; return socket-fd
void init_shot(Player *_player, int input);
void move_player(Player *_player, int input);
void gameloop(int gamesocket);			//loop where game is executed, send/recv to player takes place
void spectate(int socket, char playername[]);		//loop for spectators, where data is recived and displayed
void sig_handler();	  //if a user/system interrupts

#endif
