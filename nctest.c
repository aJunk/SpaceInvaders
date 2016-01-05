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
		int ch;
	  initscr();
	  noecho();
	  cbreak();
	  keypad(stdscr, TRUE);
	  curs_set(FALSE);
	  timeout(0);
	  resizeterm(50+2, 50+2);
//	  getmaxyx(stdscr, max_y, max_x);
	  clear();
ch = 0;
	  refresh();
	  wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');
	  // Global var `stdscr` is created by the call to `initscr()`

//BEGIN MAIN LOOP-------------------------------------------------------------
	while(1) {
	//clientside -> start
		resizeterm(50+2, 50+2);
		clear();
		wborder(stdscr, '|', '|', '-', '-', '+', '+', '+', '+');

		refresh();

		//Delay to reduce cpu-load
		//TODO: time accurately to a certain number of updates per second
		usleep(100000);

		//read user-input
		ch = wgetch(stdscr);
		mvprintw(1,1, "%d", ch);
		if(ch == 'q') break;
		

	//clientside <- end
	}
// GAME ENDS HERE --------------------------------------------------

  beep();
  endwin();
	return 0;
}
