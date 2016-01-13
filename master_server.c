/* Siple program forking itselfe whenever "p" is pressed and printing all active children to the screen.
 * Children die autmatically after 30 seconds.
 *
 * offene Punkte: Exit in Funktion zulässig?;
 */
#define _BSD_SOURCE

#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAXCHILD 10

//prototypes
int checkAlive (pid_t memcpid[]);    //checks if given children in a list (!=0) are still alive; sets dead children to "0"
void forkAndUpdate(pid_t memcpid[]); //forks process; checks if parent (-> update List) or child (-> do whatever child shoud do)


//program starts here
int main(int argc, char *argv[])
{
    pid_t memcpid[MAXCHILD]={0};  //stored child PIDs
    int numchild = 0;    //number of active child
    char c;

    printf("enter \"f\" to fork process or \"x\" to exit\n");

    while(c != 'x'){
      scanf("%c",&c);
      if(c == 'f'){
          numchild = checkAlive(memcpid);
          if (numchild >= MAXCHILD) {
              printf("maximal number of children reached (%d)\n",numchild);
          }
          else {
              forkAndUpdate(memcpid);
          }
      }
      usleep(100000);
   }

   return 0;
}

//functions
int checkAlive (pid_t memcpid[]){

  int status;
  int numchild=0;
  pid_t result;

  printf("currently running processes:\n"); //check for active children
  for(int i=0; i<MAXCHILD; i++){
      if(memcpid[i]!=0){
          result = waitpid(memcpid[i], &status, WNOHANG); //return imediately if process is still alive (Option WNOHANG)
          if (result == 0) {
                printf("%d   PID:%d\n",i+1,memcpid[i]); // Child still alive
                numchild ++;
          }
          else if (result == -1) {
                perror("waitpid");  //Error -> Exit?
          }
          else {
                memcpid[i]=0;      // Child exited
          }
      }
  }
  return numchild;

}

void forkAndUpdate(pid_t memcpid[]){

    pid_t cpid;     //new child PID
    int i;

    cpid = fork();
    if (cpid == -1) {       // fork failed
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (cpid != 0) {    // I am a parent
        for(i=0; (i<MAXCHILD) && (memcpid[i]!=0); i++); //go to next empty place
        if (i<MAXCHILD) {  //if not full
            memcpid[i]=cpid;
        }
        else {
          printf("Error: segmentation fault %d\n",i);
          exit(EXIT_FAILURE);
        }
    }
    else {             // I am a child
          usleep(5000000);
          exit(EXIT_SUCCESS);
    }
}
