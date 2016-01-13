/* Siple program forking itselfe whenever SPACE is pressed and printing all active children to the screen.
 * Children die autmatically after 30 seconds.
 */

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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


    if (argc != 2) {
    fprintf(stderr, "Usage: %s <string>\n", argv[0]);
    exit(EXIT_FAILURE);
    }

    while(1){
      scanf("%c",&c);
      if (c == ' ' && numchild < MAXCHILD) {
          cpid = fork();
          c='X';
      }
      if (cpid == -1) {    // fork failed
          perror("fork");
          exit(EXIT_FAILURE);
      }
      else if (cpid != 0) {    // I am a parent
          numchild ++;
          for(i=0; i<MAXCHILD || memcpid[i]==0; i++); //go to next empty place
          if (i<MAXCHILD) {  //if not full
              memcpid[i]=cpid;
          }
          else {
            printf("maximal number of children reached\n");
          }

          printf("currently running processes:\n"); //check for active children
          for(i=0; i<MAXCHILD; i++){
              result = waitpid(memcpid[i], &status, WNOHANG); //return imediately if process is still alive
              if (result == 0) {
                    printf("%d   PID:%d\n",i+1,memcpid[i]); // Child still alive
              } else if (result == -1) {
                    perror("waitrpid");  //Error
              } else {
                    numchild --;       // Child exited
                    memcpid[i]=0;
              }
          }


      }
      else {             // I am a child
          usleep(500);
          exit(EXIT_SUCCESS);
      }
    }
}
