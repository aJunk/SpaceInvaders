/**************************************************************************
 * SPACEINVADORS GAME - ERRORHANDLER
 * Libary for a TCP/IP based Spaceinvadors game for Linux and MacOS.
 * Provides errorhandling functionality
 *
 * written by Philipp Gotzmann, Alexander Junk and Johannes Rauer
 * UAS Technikum Wien, BMR14
 */
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
