/*Socket Client
 gcc -o client -std=c99 -D_SVID_SOURCE -Wall -Wextra -pedantic -Wno-unused-parameter socket_client.c
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
#define BUF 1024
#define PORT_MIN 1024
#define PORT_MAX 65535
#define STD_PORT 4657
#define EXIT_ERROR 1

int main(int argc, char **argv) {
	int ret = 0;
	int msgSize = -1;
	int gamesocket;
	int port = STD_PORT;
	struct sockaddr_in address;
	char ip[16] = "127.0.0.1";
	char *buf = malloc(BUF);
	
// Check arguments
	if(argc > 1)
	{
		if(strcmp(argv[1], "-i") == 0) strcpy(ip, argv[2]);
		else if (strcmp(argv[1], "-p") == 0) port = atoi(argv[2]);		//Convert string argument to int
		else if (strcmp(argv[1], "-n") == 0) ret = 5;
		else ret = 99;
	}
	if(argc > 3)
	{
		if (strcmp(argv[3], "-p") == 0) port = atoi(argv[4]);
		else if (strcmp(argv[3], "-n") == 0) ret = 5;
		else ret = 99;
	}
	if(argc > 5)
	{
		if (strcmp(argv[5], "-n") == 0) ret = 5;
		else ret = 99;
	}

	if(ret == 99)
	{
		printf("ERROR: Invalid argument! Usage: client [-i <server ip>] [-p <server port>] [-n <player name>]\n");
		return EXIT_ERROR;
	}

	if(port <= PORT_MIN || port >= PORT_MAX)
	{
		printf("ERROR: Invalid port! Port has to be between %d and %d.\n", PORT_MIN, PORT_MAX);
		return EXIT_ERROR;
	}
	
// Fill in connection information
	address.sin_family = AF_INET; 			//IPv4 protocol
	address.sin_port = htons(port); 		//Port number htons converts byte order
	ret = inet_aton(ip, &address.sin_addr);	//Convert address to bin
	
// Create Socket	Address family: AF_INET: IPv4
//					Socket type: SOCK_STREAM: Stream
//					Protocol: 0: Standard to socket type
	gamesocket = socket(AF_INET, SOCK_STREAM, 0);
	if (gamesocket < 0)
	{
		perror("Error creating socket!");
		return 1;
	}
	printf("Client Socket created\n");

// Connect to server
	ret = connect(gamesocket, (struct sockaddr*)&address, sizeof(address));
	if(ret < 0) 
	{
		perror("Error connecting to server!");
		return 1;
	}
	printf("Connected to server.\n");

// Send, Get messages
	do {
		msgSize = recv(gamesocket, buf, BUF-1, 0);
		if(msgSize == 0)
		{
			perror("Error receiving, connection closed by server!");
			return 1;
		}
		if (msgSize > 0)
		{
			buf[msgSize] = '\0';
			printf("Got message: %s\n", buf);
		}

		if (strcmp(buf, "quit\n") != 0) {
			printf ("Message to send: ");
			fgets (buf, BUF, stdin);
			ret = send(gamesocket, buf, strlen(buf), 0);
			if(ret < 0)
			{
				perror("Error sending!");
				return 1;
			}
		}
	} while (strcmp (buf, "quit\n") != 0);

// Disconnect from server
	ret = close(gamesocket);
	if(ret < 0) 
	{
		perror("Error disconnecting from server!");
		return 1;
	}	
	
	return 0;
}
