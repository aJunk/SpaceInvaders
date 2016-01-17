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

void error_handler(int error_no){	//ERRNO
	endwin();
	switch(error_no) {
		case ERR_CREATE_SOCKET: perror("Error creating socket"); break;
		case ERR_BIND: perror("Error binding. Port not free"); break;
		case ERR_LISTENER: perror("Error launching listener"); break;
		case ERR_FORK: perror("Error when forking"); break;
		
		case ERR_CONNECT: perror("Error connecting"); break;
		case ERR_DISCONNECT: perror("Error disconnecting"); break;	
		case ERR_SEND: perror("Error sending"); break;
		case ERR_RECV: perror("Error receiving"); break;
		
		case ERR_S_ARG: printf("ERROR: Invalid argument!\n\tUsage: server -p <port>\n"); break;
		case ERR_MAX_GAMES: printf("ERROR: Maximum number of games\n"); break;
		
		case ERR_C_ARG: printf("ERROR: Invalid argument!\n\tUsage: client [-i <server ip>] [-p <server port>] [-n <player name>]\n"); break;
		case ERR_INVALID_PORT: printf("ERROR: Invalid port! Port has to be between %d and %d.\n", PORT_MIN, PORT_MAX); break;
		case ERR_PLAYERNAME: printf("Error: Your playername must not be longer than %d characters\n", PLAYER_NAME_LEN); break;
		default: printf("ERROR\n");
	}
	exit(EXIT_ERROR);
}
