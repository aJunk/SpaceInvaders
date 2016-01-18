/**************************************************************************
 * SPACEINVADORS GAME - communication
 * Definitions for a TCP/IP based Spaceinvadors game for Linux and MacOS.
 * various defines, struct typedefs and declaration of global functions
 *
 * written by Philipp Gotzmann, Alexander Junk and Johannes Rauer
 * UAS Technikum Wien, BMR14
 */

#include <stdio.h>
#include <string.h>
#include "communication.h"

//for encoding and decoding packages
void handle_package(uint8_t *container, Player *player, Object obj[MX * MY], Shot shots[AMUNITION], int mode){
	uint8_t *tmp;

	if(mode == DISASSEMBLE){
    tmp = container;
    memcpy(player, tmp, sizeof(Player));
    tmp += sizeof(Player);
    memcpy(shots, tmp, sizeof(Shot) * AMUNITION);
    tmp += sizeof(Shot) * AMUNITION;

    uint16_t index = 0;
    uint16_t count = 0;

    memcpy(&count, tmp, sizeof(uint16_t));
    if(count > 0){
      for(uint16_t i = 0; i < count; i++){
        memcpy(&index, tmp + sizeof(uint16_t) + (sizeof(Object) + sizeof(uint16_t)) * i, sizeof(uint16_t));
        memcpy(&(obj[index]), tmp + sizeof(uint16_t) + sizeof(Object) * i + sizeof(uint16_t) * (i + 1), sizeof(Object));
				obj[index].status = NO_CHANGE;
      }
    }
  }else if(mode == ASSEMBLE){

    memset(container, 0, SET_SIZE_OF_DATA_EXCHANGE_CONTAINER);
    tmp = container;
    memcpy(tmp, player, sizeof(Player));
    tmp += sizeof(Player);
    memcpy(tmp, shots, sizeof(Shot) * AMUNITION);
    tmp += sizeof(Shot) * AMUNITION;
    uint16_t tmp_int = 0;
    for(uint16_t i = 0; i < MX * MY; i++){
      if(obj[i].status & UPDATED){
				//printf("updated\n");
        memcpy(tmp + sizeof(uint16_t) + (sizeof(Object) + sizeof(uint16_t)) * tmp_int, &i, sizeof(uint16_t));
        memcpy(tmp + sizeof(uint16_t) + sizeof(Object) * tmp_int + sizeof(uint16_t) * (tmp_int + 1), &(obj[i]), sizeof(Object));
        tmp_int++;
        obj[i].status = NO_CHANGE;
      }
    }
    memcpy(tmp, &tmp_int, sizeof(uint16_t));
    tmp = NULL;
	}
}
