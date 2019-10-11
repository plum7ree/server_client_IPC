#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <mqueue.h>
#include <time.h>
#include <signal.h>
#include "./snappy-c/snappy.h"
#include "./snappy-c/map.h"
#include "./snappy-c/util.h"

#include "TinyFile.h"

#define STACKSIZE 1024
#define ONE_MB 1024*1024

struct clone_arg {
    char msg[MSGSIZE];
};



size_t fread_buf( void* ptr, size_t size, FILE* stream)
{
    return fread( ptr, 1, size, stream);
}


size_t fwrite_buf( void const* ptr, size_t size, FILE* stream)
{
    return fwrite( ptr, 1, size, stream);
}

void clientHandler(void *arg) {
    // char *msg = ((struct clone_arg *)arg)->msg;
    sem_allow_transf = // sem only shared between one client and server

    
    while()
    {
	    sem_wait(sem_global); // shared with all clients

	    // 1. read request in message queue (file name, chunk size)
	    sem_post(sem_allow_msg)
	    sem_wait(sem_msg_sent)
	    msg = readMsg()
	    filenumber
	    chunksize = CHUNKSIZE(msg);

	    if(file_info_array[filenumber].filesize == 0) {
	    	// if filenumber not exist in file_info_array:
		    // 	first packet includes filenumber, chunksize, filesize
		    // 	further packet includes filenumber, chunksize
	    	file_info_array[filenumber].filesize = FILESIZE(msg);
	    	file_info_array[filenumber].cumsize = 0;
	    	chunksize = CHUNKSIZE(msg);
	    	char *buffer = malloc(FILESIZE(msg));
	    	file_info_array[filenumber].buffer = buffer;
	    }
	    sem_post(sem_msg_sent)


	    sem_post(sem_allow_transf)
	    sem_wait(sem_modif);
	    char *temp_file_buffer =  file_info_array[filenumber].buffer + cumSize

	    memcopy(temp_file_buffer, mmap_addr, chunksize);


	    file_info_array[filenumber].cumsize += chunksize;
	    totalsize = file_info_array[filenumber].filesize;

	    if (cumSize >= totalsize) {
    		break;
    	}

	}

	compress();
    

    sem_wait(sem_global); // server should acquire this to write to shared memory





    return;
}


void connectClient(void *arg) {
	// arg
	// 1. sem_allow_transf : semaphore per client and server 
	// 2. msg queue
    int clone_pid = clone(clientHandler, (void *)child_stack, SIGCHLD, (void *)arg);
}



void initTinyServer(mqd_t *mqfd, char *mmap_addr, sem_t *sem_global) {
	struct mq_attr *attr = malloc();
    attr->mq_maxmsg = 4;
    attr->mq_msgsize = MSGSIZE;
    attr->mq_flags = 0;
    *mqfd = mq_open(MQPATH, O_RDWR | O_CREAT, 0666, attr);

    if(*mqfd == -1) {
        perror("Parent mq_open failure");
        exit(0);
    }

    // int status = mq_receive(mqfd, msgbuff, 1000, 0);
    // if (status == -1) {
    //     perror("mq_receive failure\n");
    // }

    // long fileSize = (long) atoi(msgbuff);
    // printf("filesize: %lu\n", fileSize);

    // mq_close(mqfd);
    // mq_unlink(MQPATH);

    sem_global = sem_open(SEMPATH_GLOBAL , O_RDWR| O_CREAT,S_IRUSR | S_IWUSR);
    

    sem_init(sem_global, 1, 0);

    fd = shm_open(SHMPATH, O_RDWR| O_CREAT, S_IRUSR | S_IWUSR); /* Open existing object */ 
    

    if (fd == -1)
        perror("shm_open");
    /* Use shared memory object size as length argument for mmap() and as number of bytes to write() */
    if (fstat(fd, &sb) == -1)
        perror("fstat");

    mmap_addr = mmap(NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    // printf("mmap_addr: %x\n", mmap_addr);
    // // fflush(stdout);

    if (mmap_addr == MAP_FAILED)
        perror("mmap");


}

int
main(int argc, char *argv[])
{
	int fd;
	int segsize = SEGSIZE;
	char *addr;
	struct stat sb;
	sem_t *sem_global, *sem_modif;
	FILE * pFile;
	int wpid;
	int status = 0;
	
	mqd_t *mqfd = malloc();


	initTinyServer(attr, mmap_addr, sem_global);

	while(1) {
		//1. check mq for new client creation

		//2. create thread for that client (clone)
		connectClient();
	}




}

