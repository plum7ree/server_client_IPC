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

#include "TinyFile.h"

int fd;
int segsize = SEGSIZE;
char *addr;
struct stat sb;
sem_t *sem, *sem_modif;


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

    // struct   timespec tm;
    // clock_gettime(CLOCK_REALTIME, &tm);
    // tm.tv_sec += 20;

    // struct mq_attr attr;
    // attr.mq_maxmsg = 4;
    // attr.mq_msgsize = MSGSIZE;
    // attr.mq_flags = 0;
    // mqd_t mqfd = mq_open(MQPATH, O_RDWR | O_CREAT, 0666, &attr);
    // char *msgbuff = malloc(MSGSIZE);
    // if(mqfd == -1) {
    //     perror("Parent mq_open failure");
    //     exit(0);
    // }


    // int status = mq_receive(mqfd, msgbuff, 1000, 0);
    // if (status == -1) {
    //     perror("mq_receive failure\n");
    // }
    // int client_pid = atoi(msgbuff);
    // printf("client's connected: %d", client_pid);

    // mq_close(mqfd);
    // mq_unlink(MQPATH);
    // return 0;



    sem = sem_open(SEMPATH , O_RDWR| O_CREAT,S_IRUSR | S_IWUSR);
    sem_modif = sem_open(SEMPATH2 , O_RDWR| O_CREAT,S_IRUSR | S_IWUSR);
    sem_allow_transf = sem_open(SEMPATH3 , O_RDWR| O_CREAT,S_IRUSR | S_IWUSR);


    sem_init(sem, 1, 1);
    sem_init(sem_modif, 1, 0);
    sem_init(sem_allow_transf, 1, 1);

    fd = shm_open(SHMPATH, O_RDWR| O_CREAT, S_IRUSR | S_IWUSR); /* Open existing object */ 
    printf("fd: %d\n", fd);
    fflush(stdout);

    if (fd == -1)
        perror("shm_open");
    /* Use shared memory object size as length argument for mmap() and as number of bytes to write() */
    if (fstat(fd, &sb) == -1)
        perror("fstat");

    addr = mmap(NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    printf("mmap addr: %x\n", addr);
    fflush(stdout);

    if (addr == MAP_FAILED)
        perror("mmap");
    

    pFile = fopen("result.txt", "wb+");

    int cumSize;
    int fileSize;
    wait(sem_modif);
    write(STDOUT_FILENO, addr, segsize); 
    while(1) {
        sem_wait(sem_modif);
        fwrite_buf(addr, len, pFile);
        msync(addr, len, MS_SYNC);
        sem_post(sem);
        if (cumSize == fileSzie) {
            fclose(pFile);
            break;
        }
    }


    if (close(fd) == -1);
        perror("close");
    exit(EXIT_SUCCESS);
}