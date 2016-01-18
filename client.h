/**************************************************************************
 * SPACEINVADERS GAME - client (header)
 * Headerfile for a TCP/IP based Spaceinvaders game for Linux and MacOS.
 * Provides several prototypes of functions for the client.
 *
 * written by Philipp Gotzmann, Alexander Junk and Johannes Rauer
 * UAS Technikum Wien, BMR3_2014
 */
#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

int connect2server(char ip[16], int port);			//creates a socket and connects to server with given ip and port; return socket-fd
void init_shot(Player *_player, int input);
void move_player(Player *_player, int input);
void gameloop(int gamesocket);			//loop where game is executed, send/recv to player takes place
void spectate(int socket, char playername[]);		//loop for spectators, where data is recived and displayed
void sig_handler();	  //if a user/system interrupts

#endif CLIENT_H_INCLUDED
