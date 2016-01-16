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
		case -1: printf("ERROR: Invalid argument!\n\tUsage: server -p <port>\n"); break;
		case -2: printf("ERROR: Invalid port! Port has to be between %d and %d.\n", PORT_MIN, PORT_MAX); break;
		case -3: perror("Error creating socket"); break;
		case -4: perror("Error binding. Port not free"); break;
		case -5: perror("Error launching listener"); break;
		case -6: perror("Error creating new socket"); break;
		case -7: perror("Error sending"); break;
		case -8: perror("Error receiving, connection closed by client"); break;
		case -9: perror("Error disconnecting from client"); break;	
		case -11: perror("Error when forking"); break;

		case -21: printf("ERROR: Invalid argument!\n\tUsage: client [-i <server ip>] [-p <server port>] [-n <player name>]\n"); break;
		case -22: perror("Error connecting to server"); break;
		case ERR_PLAYERNAME: printf("Error: Your playername must not be longer than %d characters\n", PLAYER_NAME_LEN); break;
		case -29: perror("Error disconnecting from server"); break;
		default: printf("ERROR\n");
	}
	exit(EXIT_ERROR);
}
