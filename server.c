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

int fd;
int segsize = SEGSIZE;
char *addr;
struct stat sb;
sem_t *sem, *sem_modif, *sem_allow_transf;
FILE * pFile;
int wpid;
int status = 0;


size_t fread_buf( void* ptr, size_t size, FILE* stream)
{
    return fread( ptr, 1, size, stream);
}


size_t fwrite_buf( void const* ptr, size_t size, FILE* stream)
{
    return fwrite( ptr, 1, size, stream);
}

void run_file_transfer (void *arg) {
    char *msg = ((struct clone_arg *)arg)->msg;
    // mqd_t clientq = mq_open(msgbuff, O_RDWR, 0666, &attr);
    // volatile int i = 0;
    // volatile int a = 2;
    // volatile int b = 1;
    // for (;i< 10000000; i++) {
    //     a = a + b + 1;
    // }
    // printf("msg: %s, a:%d", msg, a);
    // fflush(stdout);

    // 1. "/sem_client_pid" ex) /sem3771
    // mq_open(/sem3771)
    // 



    return;
}



int
main(int argc, char *argv[])
{

    /****************************** msg queue ******************************************/
    // struct   timespec tm;
    // clock_gettime(CLOCK_REALTIME, &tm);
    // tm.tv_sec += 20;

    // struct mq_attr attr;
    // attr.mq_maxmsg = 4;
    // attr.mq_msgsize = MSGSIZE;
    // attr.mq_flags = 0;
    // mqd_t mqfd = mq_open(MQPATH, O_RDWR | O_CREAT, 0666, &attr);
    // char *msgbuff = malloc(MSGSIZE);

    // // printf("server pid: %d\n", getpid());

    // if(mqfd == -1) {
    //     perror("Parent mq_open failure");
    //     exit(0);
    // }


    // while(1) {
    //     int status = mq_receive(mqfd, msgbuff, 1000, 0);
    //     if (status == -1) {
    //         perror("mq_receive failure\n");
    //     }
    //     int client_pid = atoi(msgbuff);
    //     char *child_stack = malloc(STACKSIZE);
    //     child_stack += STACKSIZE; //stack top, stack grows down

    //     struct clone_arg *arg = (struct clone_arg *) malloc(sizeof(struct clone_arg));
    //     memcpy(arg->msg, msgbuff, MSGSIZE);


        
    // }

        // while ((wpid = wait(&status)) > 0);


        // mq_close(mqfd);
        // mq_unlink(MQPATH);
    
    
    // return 0;
    /****************************** msg queue ******************************************/


    /****************************** File Transfer ******************************************/

     struct   timespec tm;
    clock_gettime(CLOCK_REALTIME, &tm);
    tm.tv_sec += 20;

    struct mq_attr attr;
    attr.mq_maxmsg = 4;
    attr.mq_msgsize = MSGSIZE;
    attr.mq_flags = 0;
    mqd_t mqfd = mq_open(MQPATH, O_RDWR | O_CREAT, 0666, &attr);
    char *msgbuff = malloc(MSGSIZE);

    // printf("server pid: %d\n", getpid());

    if(mqfd == -1) {
        perror("Parent mq_open failure");
        exit(0);
    }

    int status = mq_receive(mqfd, msgbuff, 1000, 0);
    if (status == -1) {
        perror("mq_receive failure\n");
    }
    long fileSize = (long) atoi(msgbuff);
    printf("filesize: %lu\n", fileSize);

    mq_close(mqfd);
    mq_unlink(MQPATH);

    sem_modif = sem_open(SEMPATH2 , O_RDWR| O_CREAT,S_IRUSR | S_IWUSR);
    sem_allow_transf = sem_open(SEMPATH3 , O_RDWR| O_CREAT,S_IRUSR | S_IWUSR);
    

    sem_init(sem_modif, 1, 0);
    sem_init(sem_allow_transf, 1, 1);

    fd = shm_open(SHMPATH, O_RDWR| O_CREAT, S_IRUSR | S_IWUSR); /* Open existing object */ 
    

    if (fd == -1)
        perror("shm_open");
    /* Use shared memory object size as length argument for mmap() and as number of bytes to write() */
    if (fstat(fd, &sb) == -1)
        perror("fstat");

    addr = mmap(NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    // printf("mmap addr: %x\n", addr);
    // // fflush(stdout);

    if (addr == MAP_FAILED)
        perror("mmap");
    
    // pFile = fopen("result.txt", "wb+");

    long cumSize = 0;
    // long fileSize = 12; // through message queue
    char *debugbuff = malloc(segsize);
    char *tempFileBuffer = malloc(ONE_MB/2);

    /************************************ Snappy Setting *************************************************/
    char *outbuffer = xmalloc(snappy_max_compressed_length(fileSize));
    char *outbuffer2 = malloc(ONE_MB/2);
    size_t outlen = -1;
    struct snappy_env env;
    snappy_init_env(&env);
    /*****************************************************************************************************/
    
    while(1) {
        sem_wait(sem_modif);
        memcpy(debugbuff, addr, segsize);
        printf("received file seg: %s\n", debugbuff);
        // fwrite_buf(addr, segsize, pFile);
        memcpy(tempFileBuffer + cumSize, addr, segsize);
        // msync(addr, segsize, MS_SYNC);
        cumSize += segsize;
        

        if (cumSize >= fileSize) {
            // fclose(pFile);
            // TODO: compress file
            // TODO: notify client it's done
            // mq_send()
            int err = snappy_compress(&env, tempFileBuffer, fileSize, outbuffer, &outlen);       
            if (err) {
                printf("compression of failed:\n");
            }
            printf("compression done. outlen: %d\n", outlen);
            fflush(stdout);

            err = snappy_uncompress(outbuffer, outlen, outbuffer2);
            if (err) {
                printf("uncompression failed\n");
                break;
            }
            if (memcmp(outbuffer2, tempFileBuffer, fileSize)) {
                printf("failed! compression\n");
                
            } 
            else {
                printf("%s OK!\n");
            }

            break;


        }

        sem_post(sem_allow_transf);

        
    }

    // printf("file transfered succedded!\n");
    // munmap(addr, segsize);
    // shm_unlink(SHMPATH);
    // sem_unlink(SEMPATH);
    // sem_unlink(SEMPATH2);
    // sem_unlink(SEMPATH3);
    // exit(EXIT_SUCCESS);
    /****************************** File Transfer ******************************************/
}