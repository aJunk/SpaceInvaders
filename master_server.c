/* Siple program forking itselfe whenever "p" is pressed and printing all active children to the screen.
 * Children die autmatically after 30 seconds.
 */
#define _BSD_SOURCE

#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAXCHILD 10

int main(int argc, char *argv[])
{

    pid_t cpid;     //new child PID
    pid_t result;
    int numchild = 0;    //number of active childs
    int status;
    char c;
    int i;
    pid_t memcpid[MAXCHILD]={0};  //stored child PIDs


    while(1){
      scanf("%c",&c);
      if ((c == 'p') && (numchild >= MAXCHILD)) {
          printf("maximal number of children reached\n");
      }
      else if(c == 'p'){
          cpid = fork();
          c='X';
      }
      else {
          continue;
      }

      if (cpid == -1) {       // fork failed
          perror("fork");
          exit(EXIT_FAILURE);
      }
      else if (cpid != 0) {    // I am a parent
          numchild ++;
          for(i=0; (i<MAXCHILD) && (memcpid[i]!=0); i++); //go to next empty place
          if (i<MAXCHILD) {  //if not full
              memcpid[i]=cpid;
          }
          else {
            printf("Error: reached end of array");
            exit(EXIT_FAILURE);
          }

          printf("currently running processes:\n"); //check for active children
          for(i=0; i<MAXCHILD; i++){
              if(memcpid[i]!=0){
                  result = waitpid(memcpid[i], &status, WNOHANG); //return imediately if process is still alive
                  if (result == 0) {
                        printf("%d   PID:%d\n",i+1,memcpid[i]); // Child still alive
                  } else if (result == -1) {
                        perror("waitpid");  //Error -> Exit?
                  } else {
                        numchild --;       // Child exited
                        memcpid[i]=0;
                  }
              }
          }
      }
      else {             // I am a child
          usleep(5000000);
          exit(EXIT_SUCCESS);
      }
    }

    return 0;
}
