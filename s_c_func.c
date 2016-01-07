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

void error_handler(int errno){
	endwin();
	if(errno == -1) printf("ERROR: Invalid argument!\n\tUsage: server -p <port>\n");
	else if(errno == -2) printf("ERROR: Invalid port! Port has to be between %d and %d.\n", PORT_MIN, PORT_MAX);
	else if(errno == -3) perror("Error creating socket");
	else if(errno == -4) perror("Error binding. Port not free");
	else if(errno == -5) perror("Error launching listener");
	else if(errno == -6) perror("Error creating new socket");
	else if(errno == -7) perror("Error sending");
	else if(errno == -8) perror("Error receiving, connection closed by client");
	else if(errno == -9) perror("Error disconnecting from client");	

	else if(errno == -21) printf("ERROR: Invalid argument!\n\tUsage: client [-i <server ip>] [-p <server port>] [-n <player name>]\n");
	else if(errno == -22) perror("Error connecting to server");
	else if(errno == -29) perror("Error disconnecting from server");
	else printf("ERROR\n");
	exit(EXIT_ERROR);
}
