#include "communication.h"
#include <sys/types.h>
#include <ncurses.h>

#ifndef GRAPHX_H_INCLUDED
#define GRAPHX_H_INCLUDED

extern WINDOW* fieldscr;
extern WINDOW* statscr;
extern WINDOW* scorescr;
extern WINDOW* infoscr;


void init_graphix();
void frame_change();
void draw_obj(Object obj[MX * MY], char character);
void draw_player(Player *_player,  char character);
void draw_shot(Shot _shots[AMUNITION], char character);
void print_scorescr(char playername[PLAYER_NAME_LEN + 1], int16_t score, int16_t life, int spectators);
void print_statscr(char* str);
int disp_infoscr(char mode);

#endif
