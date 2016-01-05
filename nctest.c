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

void frame_change();

int main(int argc, char **argv) {
		int ch;

		initscr();
		keypad(stdscr, TRUE);
	  noecho();
	  cbreak();
	  curs_set(FALSE);
	  timeout(0);
	  resizeterm(50+2, 50+2);
/*
		initscr();
        curs_set(FALSE);
        cbreak();
        timeout(0);
        keypad(stdscr, TRUE);
//	  getmaxyx(stdscr, max_y, max_x);
	  clear();
		*/
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

		frame_change();
		//Delay to reduce cpu-load
		//TODO: time accurately to a certain number of updates per second
		usleep(100000);

		//read user-input
		ch = getch();
		mvprintw(2,1, "Hello");
		mvprintw(1,1, "%d", ch);
		refresh();
		if(ch == 'q') break;


	//clientside <- end
	}
// GAME ENDS HERE --------------------------------------------------

  beep();
  endwin();
	return 0;
}


void frame_change(){
  static char toggle = 0;

  if(toggle){
    mvprintw(0, 0, "+");
    toggle = 0;
  }else{
    mvprintw(0, 0, "-");
    toggle = 1;
  }
}
