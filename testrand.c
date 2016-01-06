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

//server-variables
Object s_obj[MX * MY] = {{{0,0}, 0, 0, 0}};

//serverside functions
void place_objects(Object obj[MX * MY], int lines);

int main(int argc, char **argv) {

	s_obj[3].pos[0] = 4;
	s_obj[3].pos[1] = 12;
	s_obj[3].life = 6;
	s_obj[3].status = UPDATED;

	s_obj[17].pos[0] = 5;
	s_obj[17].pos[1] = 8;
	s_obj[17].life = 6;
	s_obj[17].status = UPDATED;

	place_objects(s_obj, 1);
	printf("--OBJECTS--\n");
	for(int i = 0; i < 50; i++){
		printf("\nOBJ: %d\n", i);
		printf("POS: %d, %d\n", s_obj[i].pos[0], s_obj[i].pos[1]);
		//printf("LIFE: %d\n", s_obj[i].life);
		//printf("STAT: %d\n\n", s_obj[i].status);
	}

	return 0;
}

void place_objects(Object obj[MX * MY], int lines){
	int objn = 0;

	for(int i = 0; i < lines; i++){				//do it for given number of lines
		for(int j = 3; j < (MX-2); j++){		//do it from 3rd position in row to 3rd-last position in row
			while(s_obj[objn].life != 0){		//search for dead objects
				objn++;
			}
			s_obj[objn].pos[0] = j;
			s_obj[objn].pos[1] = i;
			s_obj[objn].life = 4;
			s_obj[objn].status = UPDATED;
			objn++;
		}
	}

}
