#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>






int main(){
  char buffer[] = "shot";

  //mkfifo("S_QUEUE", 0666);


  int sound_queue = open("S_QUEUE", O_RDWR);

    if (sound_queue == -1) {
        perror("error_fifo yould not be opened!");
        exit(-1);
    }


    write(sound_queue, buffer, strlen(buffer));

    close(sound_queue);

    return 0;

}
