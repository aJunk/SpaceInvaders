#include "communication.h"
#include <sys/types.h>
#include <ncurses.h>

#ifndef GRAPHX_H_INCLUDED
#define GRAPHX_H_INCLUDED

extern WINDOW* fieldscr;
extern WINDOW* statscr;

void init_graphix();
void frame_change();
void draw_obj(Object obj[MX * MY], char character);
void draw_player(Player *_player,  char character);
void draw_shot(Shot _shots[AMUNITION], char character);



#endif
