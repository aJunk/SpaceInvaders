/*Socket Server
 gcc -o server -std=c99 -Wall -Wextra -pedantic -Wno-unused-parameter socket_server.c
 Nachempfunden:
 http://openbook.rheinwerk-verlag.de/c_von_a_bis_z/025_c_netzwerkprogrammierung_005.htm#mj80e21e66b25fefd05e8c817719845013
 Broken Socket implementation
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "communication.h"

int main(int argc, char **argv) {
	int ret = 0;
	int msgSize = -1;
	int gamesocket, new_gamesocket;
	int port = STD_PORT;
	struct sockaddr_in address;
	char *buf = malloc(BUF);
	socklen_t addrLength;

// Check arguments
	if(argc > 2)
	{
		if(strcmp(argv[1], "-p") == 0)
		{
			port = atoi(argv[2]);			//Convert string argument to int
		}
		else
		{
			printf("ERROR: Invalid argument! Usage: server -p <port>\n");
			return EXIT_ERROR;
		}
	}
	
	if(port <= PORT_MIN || port >= PORT_MAX)
	{
		printf("ERROR: Invalid port! Port has to be between %d and %d.\n", PORT_MIN, PORT_MAX);
		return EXIT_ERROR;
	}	
	
// Fill in connection information
	address.sin_family = AF_INET;			//IPv4 protocol
	address.sin_addr.s_addr = INADDR_ANY; 	//Receive packets from any address
	address.sin_port = htons(port);		//Port number htons converts byte order	
	
// Create Socket	Address family: AF_INET: IPv4
//					Socket type: SOCK_STREAM: Stream
//					Protocol: 0: Standard to socket type
	gamesocket = socket (AF_INET, SOCK_STREAM, 0);
	if (gamesocket == -1)
	{
		perror("Error creating socket!");
		return -1;
	}
	printf("Server Socket created.\n");

// Bind Socket to process
	ret = bind(gamesocket, (struct sockaddr*)&address, sizeof(address));
	if(ret < 0)
	{
		perror("Error binding! Port not free.");
		return 1;
	}
	printf("Bind successfully.\n");

// Make listener (queue) for new connections
	ret = listen(gamesocket, 5);		//max. 5 connections
	if(ret < 0)
	{
		perror("Error making listener!");
		return 1;
	}
	printf("Listener initialized.\n");

	while(1)
	{
// Get next connection in queue
		addrLength = sizeof(address);
		new_gamesocket = accept(gamesocket, (struct sockaddr *) &address, &addrLength);
		if(new_gamesocket < 0)
		{
			perror("Error creating new socket!");
			return 1;
		}
		printf ("New Client connected: %s\n", inet_ntoa (address.sin_addr));

// Send, Get messages
		do {
			if (strcmp(buf, "quit\n") != 0) {
				printf ("Message to send: ");
				fgets (buf, BUF, stdin);
				ret = send(new_gamesocket, buf, strlen(buf), 0);
				if(ret < 0)
				{
					perror("Error sending!");
					return 1;
				}
			}
			if (strcmp(buf, "quit\n") == 0) {
				ret = close(new_gamesocket);
				if(ret < 0)
				{
					perror("Error sending!");
					return 1;
				}
				strcpy(buf, "\0");
				printf("Connection closed.\n");
				break;
			}

			msgSize = recv(new_gamesocket, buf, BUF-1, 0);
			if(msgSize == 0)
			{
				perror("Error receiving, connection closed by client!");
				return 1;
			}
			if (msgSize > 0)
			{
				buf[msgSize] = '\0';
				printf("Got message: %s\n", buf);
			}
		} while (strcmp (buf, "quit\n") != 0);
	}

// Disconnect from client
	ret = close(gamesocket);
	if(ret < 0)
	{
		perror("Error disconnecting from client!");
		return 1;
	}
	return 0;
}
