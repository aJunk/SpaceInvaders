server:
	gcc -o server -std=c99 -Wall -Wextra -pedantic -Wno-unused-parameter socket_server.c

client:
	gcc -o client -std=c99 -D_BSD_SOURCE -Wall -Wextra -pedantic -Wno-unused-parameter socket_client.c