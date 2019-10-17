#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char * argv[]){   
	int server_pid, client_pid;
	int status;
	int pid;
	const int numseg[3] = {1,3,5};
	const int segsize[5] = {32,64,128,256,512};

	int n_numseg = 3;
	int n_segsize = 5;

	char argbuff[30];
	snprintf(argbuff, 30, "--n_sms %d --sms_size %d", numseg[0], segsize[0]);
	execvp("./server", argbuff);

	// for (int i =0; i < n_numseg; i++) {
	// 	for (int j=0; j < n_segsize; j++) {
	// 		server_pid = fork();

	// 		if (server_pid == 0){
	// 			/* CHILD */

	// 			printf("server\n");

	// 			char argbuff[30];
	// 			snprintf(argbuff, 30, "--n_sms %d --sms_size %d", numseg[i], segsize[j]);
	// 			execvp("./server", argbuff);
	// 			//only get here if exec failed                                                                                                                                             
	// 			perror("execve failed");
	// 		} else if (server_pid > 0){
	// 			/* PARENT */
	// 			usleep(500);

	// 			client_pid = fork();
	// 			if (client_pid == 0) {
	// 				execvp("./client", "--conf sample.yaml");
	// 				//only get here if exec failed                                                                                                                                             
	// 				perror("execve failed");
	// 			} 
	// 			else {
	// 				// wait client to complete
	// 				if((pid = wait(&status)) < 0){
	// 					perror("error on wait\n");
	// 					_exit(1);
	// 				} 
	// 			}

	// 			// wait server to compete
	// 			if((pid = wait(&status)) < 0){
	// 					perror("error on wait\n");
	// 					_exit(1);
	// 			}

	// 		} else {
	// 			perror("fork failed");
	// 			_exit(1);
	// 		}
	// 	}
	// }
	

	return 0; //return success
}