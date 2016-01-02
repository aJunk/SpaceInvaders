#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#define PORT_MIN 1024
#define PORT_MAX 65535
#define STD_PORT 4657
#define EXIT_ERROR 1

//ncurses color functions
#define bkg_colour 2
#define obj_colour 1
#define player_colour 3

#define DELAY 25000

//BITMASK for Player
#define UP 128
#define DOWN 64
#define LEFT 32
#define RIGHT 16
#define INIT_SHOT 8
//TODO: mask for request_pause/request_resume (same bit?), restart, quit game

//Package handling flags
#define ASSEMBLE 1
#define DISASSEMBLE 0

#define UPDATED 128
#define NO_CHANGE 0

//FIELD width & height
#define MX 50
#define MY 10

//size of amunition array
#define AMUNITION 1

//size of data eschange containers (needs to be known at pre-compilation-time)
#define SET_SIZE_OF_DATA_EXCHANGE_CONTAINER sizeof(Object) * MX * MY + sizeof(Player) + sizeof(int) * MX * MY + sizeof(int) * 3


typedef struct {
  char width;
  char height;
  char* symbols;
}Art;

typedef struct {
  int pos[2];
  int type;
  int life;
  //Art* art;
  char status;
}Object;

typedef struct {
  int pos[2];
  int modifier;
  int life;
  int amunition;
  char instructions;
}Player;

typedef struct{
  int pos[2];
  int active;
}Shot;

//global functions
void handle_package(char *container, Player *player, Object obj[MX * MY], Shot shots[AMUNITION], int mode);

#endif
