#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char * argv[]){   
  int server_pid, client_pid;
  int status;

  server_pid = fork();

  if (server_pid == 0){
    /* CHILD */

    printf("server\n");

    execvp("./server", "--n_sms 5 --sms_size 32");
    //only get here if exec failed                                                                                                                                             
    perror("execve failed");
  }else if (server_pid > 0){
    /* PARENT */
    sleep(1);

    client_pid = fork();
    if (client_pid == 0) {
      execvp("./client", "--conf sample.yaml");
      //only get here if exec failed                                                                                                                                             
      perror("execve failed");
    } 
    else {
      if((pid = wait(&status)) < 0){
        perror("error on wait\n");
        _exit(1);
      }
      
    }
  }

  

    printf("Parent: finished\n");

  }else{
    perror("fork failed");
    _exit(1);
  }

  return 0; //return success
}