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

#include "TinyFile.h"


// link with -lrt -pthread


size_t fread_buf( void* ptr, size_t size, FILE* stream)
{
    return fread( ptr, 1, size, stream);
}


size_t fwrite_buf( void const* ptr, size_t size, FILE* stream)
{
    return fwrite( ptr, 1, size, stream);
}


int
main(int argc, char *argv[])
{   
    int r;
    int fd;
    size_t len = SEGSIZE;
    char *addr;
    char *buff = malloc(len);
    /* Size of shared memory object */
    FILE * pFile;
    sem_t *sem, *sem_modif, *sem_allow_transf;


    /****************************** msg queue ******************************************/
    // int pid = getpid();
    // printf("client pid: %d", pid);



    // mqd_t mqfd = mq_open(MQPATH, O_WRONLY, 0666, NULL);
    // if(mqfd == -1) {
    //     perror("Child mq_open failure");
    //     exit(0);
    // }

    // int PIDSIZE = 8;
    // char *msgbuff = malloc(PIDSIZE);
    // sprintf(msgbuff, "%d", pid);

    // // mqd_t recvq = mq_open(msgbuff, O_WRONLY | O_CREAT, 0666, NULL); // client's pid becomes receving mq from server



    // int status = mq_send(mqfd, msgbuff, MSGSIZE, 0);
    // if (status == -1) {
    //     perror("mq_send failure\n");
    // }

    // mq_close(mqfd);
    // printf("Child process done\n");

    // // mq_receive(recvq, msgbuff, 1000, 0);
    // // mq_close(recvq);
    

    // return 0;
    /****************************** msg queue ******************************************/

    /****************************** File Transfer ******************************************/
    int pid = getpid();
    printf("client pid: %d\n", pid);



    mqd_t mqfd = mq_open(MQPATH, O_WRONLY, 0666, NULL);
    if(mqfd == -1) {
        perror("Child mq_open failure");
        exit(0);
    }

    int PIDSIZE = 8;
    char *msgbuff = malloc(PIDSIZE);

     pFile = fopen(filepath, "rb");
    if (pFile==NULL) {
        fputs ("File error",stderr); 
        exit (1);
    }
    // obtain file size:
    fseek (pFile , 0 , SEEK_END);
    long fileSize = ftell (pFile);
    sprintf(msgbuff, "%lu", fileSize);
    rewind (pFile);
    printf("file size: %lu\n", fileSize);

    int status = mq_send(mqfd, msgbuff, MSGSIZE, 0);
    if (status == -1) {
        perror("mq_send failure\n");
    }

    mq_close(mqfd);

    sem_modif = sem_open(SEMPATH2 , O_RDWR,S_IRUSR | S_IWUSR);
    sem_allow_transf = sem_open(SEMPATH3 , O_RDWR,S_IRUSR | S_IWUSR);

    if (sem == -1){
        perror("sem_open");
    }

    fd = shm_open(SHMPATH, O_RDWR , 0);
    if (fd == -1)
        perror("shm_open");

    if (ftruncate(fd, len) == -1)
        perror("ftruncate failed");
                                                /* Open existing object */
    addr = mmap(NULL, len, PROT_WRITE, MAP_SHARED, fd, 0); 
    if (addr == MAP_FAILED)
            perror("mmap");
    

    printf("mmap addr: %x\n", addr);
    fflush(stdout); // printf doesn't print exactly when is was called.

    // int value = -2;
    // sem_getvalue(sem_modif, &value);
    // printf("sem val: %d", value);

   

    char *debugbuff = malloc(len);
    long cumSize = 0;
    while(1) {
        sem_wait(sem_allow_transf);
        printf("writen to buffer\n");

        memset(addr, 0, len);
        int res = fread_buf(addr, len, pFile);

        cumSize += len;
        
        
        r = sem_post(sem_modif);

        if (cumSize >= fileSize) {
            fclose(pFile);
            break;
        }
        

    }   


    
    munmap(addr, len);
    shm_unlink(SHMPATH);
    sem_unlink(SEMPATH);
    sem_unlink(SEMPATH2);
    sem_unlink(SEMPATH3);
    if (close(fd) == -1)
        perror("close"); /* 'fd' is no longer needed */
    // exit(EXIT_SUCCESS);
    /****************************** File Transfer ******************************************/
}


