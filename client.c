

#include "TinyFile.h"




int initGlobalMessageQueue(mqd_t *mqfd_ptr) {
    // client sends private msq path, private semaphores path
    mqd_t fd = mq_open(MQPATH_GLOBAL, O_RDWR);
    printf("1\n");
    if(fd == -1) {
        perror("global mq_open failure");
        exit(0);
    }
    *mqfd_ptr = fd;

    return fd;
}

int initPrivateMessageQueue(mqd_t *mqfd_ptr, int index) {
    int pid = getpid();
    char msgbuff[PRIV_MSG_PATH_SIZE];
    sprintf(msgbuff, "/%04d%03d", pid, index); // ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//

    struct mq_attr attr;
    attr.mq_maxmsg = MAXMSGNUM;
    attr.mq_msgsize = MSGSIZE_PRIVATE;
    attr.mq_flags = 0;

    mqd_t fd = mq_open(msgbuff, O_RDWR | O_CREAT, 0666, &attr);

    if(fd == -1) {
        perror("private mq_open failure");
        exit(0);
    }

    *mqfd_ptr = fd;

    return fd;
}

void initSemaphore(sem_t **sem_ptr, int index) {
    int pid = getpid();
    char msgbuff[PRIV_SEM_PATH_SIZE];
    sprintf(msgbuff, "/%04d%03d", pid, index);
    sem_t *sem = sem_open(msgbuff , O_RDWR | O_CREAT ,S_IRUSR | S_IWUSR);

    if (sem == SEM_FAILED){
        perror("sem_open");
    }

    *sem_ptr = sem;
}


void initSharedMem(int *shm_fd, char **shm_addr, int len) {
    int fd = shm_open(SHMPATH, O_RDWR , 0);
    if (fd == -1)
        perror("shm_open");

    if (ftruncate(fd, len) == -1)
        perror("ftruncate failed");
                                                /* Open existing object */
    char *addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    if (addr == MAP_FAILED)
        perror("mmap");
    

    printf("mmap addr: %x\n", addr);
    fflush(stdout); // printf doesn't print exactly when is was called.

    *shm_fd = fd;
    *shm_addr = addr;

}


void initClient(shm_info_t *shm_info, client_sem_t *clsem, client_mqfd_t *mqfd) {
    // 1. open global msg queue
    // 1. init two private message queue (send to server, receive from server)
    // 1. init sharedmemory
    // 1. init semaphores 
    printf("1\n");
    initGlobalMessageQueue(&mqfd->mqfd_global);
    printf("1\n");
    initPrivateMessageQueue(&mqfd->mqfd_to_server, MQ_TO_SERVER_INDEX);
    printf("1\n");
    initPrivateMessageQueue(&mqfd->mqfd_from_server, MQ_FROM_SERVER_INDEX);
    printf("1\n");

    shm_info->len = SEGSIZE;
    initSharedMem(&shm_info->fd, &shm_info->addr, shm_info->len);
    initSemaphore(&clsem->sem_modif, SEM_MODIF_INDEX);
    initSemaphore(&clsem->sem_allow_transf, SEM_ALLOW_TRANSF_INDEX);
    // initSemaphore(clsem->sem_allow_to_server_msg , 2);
    // initSemaphore(clsem->sem_to_server_sent , 3);
    // initSemaphore(clsem->sem_allow_from_server_msg , 4);
    // initSemaphore(clsem->sem_from_server_sent , 5);

}

void connectToServer(client_mqfd_t *mqfd, client_sem_t *clsem) {
    // PROTOCOL : send PID to server. server will mq_open("/%04d%03d"), sem_open("/%04d%03d")
    int pid = getpid();
    char msgbuff[MSGSIZE_GLOBAL];
    sprintf(msgbuff, "%04d", pid);
    int status = mq_send(mqfd->mqfd_global, msgbuff, MSGSIZE_GLOBAL, 0);
    if(status == -1){
        perror("global mq_send failed\n");
        exit(0);
    }
    printf("message with pid:%d has been sent to global queue\n",pid);

}




/* Size of shared memory object */
// FILE * pFile;
// sem_t *sem, *sem_modif, *sem_allow_transf;
// client_mqfd_t client_mqfd;


int
main(int argc, char *argv[])
{   
    client_mqfd_t mqfd;

    client_sem_t clsem;
    shm_info_t shm_info;
    initClient(&shm_info, &clsem, &mqfd);

    connectToServer(&mqfd, &clsem);

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
    // int pid = getpid();
    // printf("client pid: %d\n", pid);



    // mqd_t mqfd = mq_open(MQPATH, O_WRONLY, 0666, NULL);
    // if(mqfd == -1) {
    //     perror("Child mq_open failure");
    //     exit(0);
    // }

    // int PIDSIZE = 8;
    // char *msgbuff = malloc(PIDSIZE);

    //  pFile = fopen(filepath, "rb");
    // if (pFile==NULL) {
    //     fputs ("File error",stderr); 
    //     exit (1);
    // }
    // // obtain file size:
    // fseek (pFile , 0 , SEEK_END);
    // long fileSize = ftell (pFile);
    // sprintf(msgbuff, "%lu", fileSize);
    // rewind (pFile);
    // printf("file size: %lu\n", fileSize);

    // int status = mq_send(mqfd, msgbuff, MSGSIZE, 0);
    // if (status == -1) {
    //     perror("mq_send failure\n");
    // }

    // mq_close(mqfd);

    // sem_modif = sem_open(SEMPATH2 , O_RDWR,S_IRUSR | S_IWUSR);
    // sem_allow_transf = sem_open(SEMPATH3 , O_RDWR,S_IRUSR | S_IWUSR);

    // if (sem == -1){
    //     perror("sem_open");
    // }

    // fd = shm_open(SHMPATH, O_RDWR , 0);
    // if (fd == -1)
    //     perror("shm_open");

    // if (ftruncate(fd, len) == -1)
    //     perror("ftruncate failed");
    //                                             /* Open existing object */
    // addr = mmap(NULL, len, PROT_WRITE, MAP_SHARED, fd, 0); 
    // if (addr == MAP_FAILED)
    //         perror("mmap");
    

    // printf("mmap addr: %x\n", addr);
    // fflush(stdout); // printf doesn't print exactly when is was called.

    // // int value = -2;
    // // sem_getvalue(sem_modif, &value);
    // // printf("sem val: %d", value);

   

    // char *debugbuff = malloc(len);
    // long cumSize = 0;
    // while(1) {
    //     sem_wait(sem_allow_transf);
    //     printf("writen to buffer\n");

    //     memset(addr, 0, len);
    //     int res = fread_buf(addr, len, pFile);

    //     cumSize += len;
        
        
    //     r = sem_post(sem_modif);

    //     if (cumSize >= fileSize) {
    //         fclose(pFile);
    //         break;
    //     }
        

    // }   


    
    // munmap(addr, len);
    // shm_unlink(SHMPATH);
    // sem_unlink(SEMPATH);
    // sem_unlink(SEMPATH2);
    // sem_unlink(SEMPATH3);
    // if (close(fd) == -1)
    //     perror("close"); /* 'fd' is no longer needed */
    // exit(EXIT_SUCCESS);
    /****************************** File Transfer ******************************************/
}


