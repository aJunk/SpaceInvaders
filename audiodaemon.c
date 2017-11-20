#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>






int main(){
  char buffer[256];

  mkfifo("S_QUEUE", 0666);


  int sound_queue = open("S_QUEUE", O_RDONLY);

    if (sound_queue == -1) {
        perror("error_fifo yould not be opened!");
        exit(-1);
    }

    while (strcmp(buffer, "END")) {

        while(3 > read(sound_queue, buffer, 265));
        if(!strcmp(buffer, "hit")){
          system("afplay ./fx/explosion-04.wav ");
        }else if(!strcmp(buffer, "shot")){
          //system("afplay ./fx/shoot-03.wav ");
        }
        printf("%s", buffer);

    }

    return 0;

}
