#include "communication.h"
#include <sys/types.h>
#include <ncurses.h>

#ifndef GRAPHX_H_INCLUDED
#define GRAPHX_H_INCLUDED

//ncurses color functions
#define bkg_colour 2
#define obj_colour 1
#define player_colour 3
#define gray_colour 4
#define obj_2_color 5

//FIELDS - MAIN FIELD in communication.h
//STAT-SCREEN width & height
#define STAT_MX 50
#define STAT_MY 2

//SCORE-SCREEN width & height
#define SCORE_MX 50
#define SCORE_MY 2

//INFO-SCREEN width & height
#define INFO_MX 30
#define INFO_MY 6

//modes for clearing windows
#define LEAVE_BORDER 0
#define NO_BORDER 1

//object lines
#define OBJ_LINE_L_OFFSET 3
#define OBJ_LINE_R_OFFSET 3

#define HEIGHT_OF_PLAYER_SPACE 2

//Special characters for non-standard objects
#define OP_SHOT '$'
#define POWER_UP 'U'

extern WINDOW* fieldscr;
extern WINDOW* statscr;
extern WINDOW* scorescr;
extern WINDOW* infoscr;


void init_graphix();
void print_scorescr(char playername[PLAYER_NAME_LEN + 1], int16_t score, int16_t life, int spectators);		//top of field: prints playername, score and life
void print_statscr();																		//bottom of field: prints possible commands
int disp_infoscr(char mode);																//shows message on gameover/restart/quit; returns command entered by player
void print_server_msg(pid_t pid, char status, char msg[], int i_info, char s_info[]);		//server logging; prints pid, SUCCESS/INFO/ERROR and a given message (optional int and infostr)
void frame_change();
void draw_obj(Object obj[MX * MY], char character);
void draw_player(Player *_player,  char character);
void draw_shot(Shot _shots[AMUNITION], char character);
void draw_line(WINDOW* scr, uint16_t height_from_bottom);

#endif
