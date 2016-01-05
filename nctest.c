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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "communication.h"


int main(int argc, char **argv) {
		int ch = 0;
	  initscr();
	  noecho();
	  cbreak();
	  keypad(stdscr, TRUE);
	  curs_set(FALSE);
	  timeout(0);
	  resizeterm(MY+2, MX+2);
	  getmaxyx(stdscr, max_y, max_x);
	  clear();

	  refresh();
	  wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');
	  // Global var `stdscr` is created by the call to `initscr()`

//BEGIN MAIN LOOP-------------------------------------------------------------
	while(1) {
	//clientside -> start
		resizeterm(MY+2, MX+2);
		clear();
		wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');

		refresh();

		//Delay to reduce cpu-load
		//TODO: time accurately to a certain number of updates per second
		usleep(DELAY);

		//read user-input
		ch = wgetch(stdscr);

		if(ch == 'q') break;
		printf("%d\n", c_player.instructions);

	//clientside <- end
	}
// GAME ENDS HERE --------------------------------------------------

  beep();
  endwin();
	return 0;
}
