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

void error_handler(int error_no){
	endwin();
	if(error_no == -1) printf("ERROR: Invalid argument!\n\tUsage: server -p <port>\n");
	else if(error_no == -2) printf("ERROR: Invalid port! Port has to be between %d and %d.\n", PORT_MIN, PORT_MAX);
	else if(error_no == -3) perror("Error creating socket");
	else if(error_no == -4) perror("Error binding. Port not free");
	else if(error_no == -5) perror("Error launching listener");
	else if(error_no == -6) perror("Error creating new socket");
	else if(error_no == -7) perror("Error sending");
	else if(error_no == -8) perror("Error receiving, connection closed by client");
	else if(error_no == -9) perror("Error disconnecting from client");	
	else if(error_no == -11) perror("Error when forking");

	else if(error_no == -21) printf("ERROR: Invalid argument!\n\tUsage: client [-i <server ip>] [-p <server port>] [-n <player name>]\n");
	else if(error_no == -22) perror("Error connecting to server");
	else if(error_no == -29) perror("Error disconnecting from server");
	else printf("ERROR\n");
	exit(EXIT_ERROR);
}
