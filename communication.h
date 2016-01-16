#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef COMMUNICATION_H
#define COMMUNICATION_H


#ifdef NOSOUND
#define SOUND 0
#endif

#ifdef HAVESOUND
#define SOUND 1
#endif


#ifdef __linux__
#define PLAYME_EXPLOSION "aplay ./fx/explosion-04.wav &"
#define PLAYME_SHOT "aplay ./fx/shoot-03.wav &"
#endif

#ifdef __APPLE__
#define PLAYME_EXPLOSION "afplay ./fx/explosion-04.wav &"
#define PLAYME_SHOT "afplay ./fx/shoot-03.wav &"
#endif

#define PORT_MIN 1024
#define PORT_MAX 65535
#define STD_PORT 4657
#define NUM_CONNECTIONS 5
#define EXIT_ERROR 1
#define EXIT_SUCCESS 0

//ncurses color functions
#define bkg_colour 2
#define obj_colour 1
#define player_colour 3
#define gray_colour 4
#define obj_2_color 5

#define DELAY 50000

//BITMASK for Player
#define UP 128
#define DOWN 64
#define LEFT 32
#define RIGHT 16
#define INIT_SHOT 8
#define QUIT 4
#define RESTART 2
//TODO: mask for request_pause/request_resume (same bit?), restart, quit game

//Package handling flags
#define ASSEMBLE 1
#define DISASSEMBLE 0

#define UPDATED 128
#define NO_CHANGE 0

//FIELD width & height
#define MX 50
#define MY 10

//STAT-SCREEN width & height
#define STAT_MX 50
#define STAT_MY 3

//SCORE-SCREEN width & height
#define SCORE_MX 50
#define SCORE_MY 2

//INFO-SCREEN width & height
#define INFO_MX 30
#define INFO_MY 6

//modes for clearing windows
#define LEAVE_BORDER 0
#define NO_BORDER 1

//size of amunition array
#define AMUNITION 1

//size of data exchange containers (needs to be known at pre-compilation-time)
#define SET_SIZE_OF_DATA_EXCHANGE_CONTAINER sizeof(Object) * MX * MY + sizeof(Player) + sizeof(uint16_t) * MX * MY + sizeof(uint16_t) * 3

//object lines
#define OBJ_LINE_L_OFFSET 3
#define OBJ_LINE_R_OFFSET 3

#define PLAYER_NAME_LEN 10

#define HEIGHT_OF_PLAYER_SPACE 2

//ERRORS
#define ERR_PLAYERNAME -23

//Special characters for non-standard objects
#define OP_SHOT '$'
#define POWER_UP 'U'

typedef struct{
    char name[PLAYER_NAME_LEN + 1];
    pid_t pid;
    int port;
}Client;

typedef struct {
  char width;
  char height;
  char* symbols;
}Art;

typedef struct {
  uint16_t pos[2];
  uint8_t type;
  int16_t life;
  //Art* art;
  uint8_t status;
}Object;

typedef struct {
  uint16_t pos[2];
  uint8_t modifier;
  int16_t life;
  int16_t score;
  uint8_t amunition;
  uint8_t instructions;
}Player;

typedef struct{
  uint16_t pos[2];
  uint8_t active;
}Shot;

//global functions
void handle_package(char *container, Player *player, Object obj[MX * MY], Shot shots[AMUNITION], int mode);
void error_handler(int error_no);

#endif
