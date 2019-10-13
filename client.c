

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
    char msgbuff[PRIV_MQ_PATH_SIZE];

    snprintf(msgbuff,PRIV_MQ_PATH_SIZE ,"/%0*d%0*d", (int)PATH_PID_SIZE, pid, (int)PATH_INDEX_SIZE, index);// ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//
    // index 0 : msg queue from cleint to the server, 
    // index 1 : msg queue from the server to client

    struct mq_attr attr;
    attr.mq_maxmsg = MAXMSGNUM;
    attr.mq_msgsize = MSGSIZE_PRIVATE;
    attr.mq_flags = 0;

    mqd_t fd = mq_open(msgbuff, O_RDWR | O_CREAT, 0666, &attr);

    if(fd == -1) {
        perror("private mq_open failure");
        exit(0);
    }
    printf("mq path: %s\n", msgbuff);
    *mqfd_ptr = fd;

    return fd;
}

void initSemaphore(sem_t **sem_ptr, int index) {
    int pid = getpid();
    char msgbuff[PRIV_SEM_PATH_SIZE];
    snprintf(msgbuff, PRIV_SEM_PATH_SIZE,"/%0*d%0*d", (int)PATH_PID_SIZE, pid, (int)PATH_INDEX_SIZE, index);// ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//
    sem_t *sem = sem_open(msgbuff , O_RDWR | O_CREAT ,S_IRUSR | S_IWUSR, 0);
    // index 0 : sem_allow_transf, 
    // index 1 : sem_modif


    if (sem == SEM_FAILED){
        perror("sem_open");
    }
    printf("sem path: %s\n", msgbuff);
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
    // pid
    initGlobalMessageQueue(&mqfd->mqfd_global);
    //meta data of the file (how many chunks)
    // fist msg :| filenumber(4) | filesize(4)  |
    // other msg:| filenumber(4) | chunksize(4) |
    initPrivateMessageQueue(&mqfd->mqfd_to_server, MQ_TO_SERVER_INDEX);
    //TODO: remove 
    //2 semaphores per client
    initPrivateMessageQueue(&mqfd->mqfd_from_server, MQ_FROM_SERVER_INDEX);

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
    *(int*)msgbuff = pid; // we are sending integer, not string!
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

void sendFile(char *path, shm_info_t *shm_info, client_mqfd_t *mqfd, client_sem_t *clsem, int filenumber) {

    char msgbuff[MSGSIZE_PRIVATE];
    char chunkbuff[SEGSIZE];
    FILE *pFile;
    int cumsize = 0;

    pFile = fopen(path, "rb");
    if (pFile==NULL) {
        perror("file open error!\n");
    }

    // obtain file size:
    fseek (pFile , 0 , SEEK_END);
    unsigned long filesize = ftell (pFile);
    sprintf(chunkbuff, "%lu", filesize);
    rewind (pFile);
    printf("file size: %lu\n", filesize);


    // protocol : | filenumber (4bytes) | filesize (8) |
    *((int *)msgbuff) = filenumber;
    *((unsigned long *)msgbuff + HEADER_FNO) = filesize;

    int status = mq_send(mqfd->mqfd_to_server , msgbuff, MSGSIZE_PRIVATE,0);
    if (status == -1) {
        perror("mq_send failure\n");
    }
    printf("first msg sent!\n");
    

    while(cumsize < filesize) {
        unsigned long chunksize = fread_buf(chunkbuff, SEGSIZE, pFile);
        *((int *)msgbuff) = filenumber;
        *((unsigned long *)msgbuff + HEADER_FNO) = chunksize;

        int status = mq_send(mqfd->mqfd_to_server , msgbuff, MSGSIZE_PRIVATE,0);
        if (status == -1) {
            perror("mq_send failure\n");
        }
        printf("chunk msg sent!\n");

        sem_wait(clsem->sem_allow_transf);
        memset(shm_info->addr, 0, SEGSIZE);
        memcpy(shm_info->addr, chunkbuff, chunksize);
        printf("file sent! filenumber: %d chunksize: %lu\n", filenumber, chunksize);
        sem_post(clsem->sem_modif);

        cumsize += chunksize;
    }
    

}

int
main(int argc, char *argv[])
{   
    client_mqfd_t mqfd;

    client_sem_t clsem;
    shm_info_t shm_info;
    initClient(&shm_info, &clsem, &mqfd);

    connectToServer(&mqfd, &clsem);

    int filenumber = 0;
    char *filepath = "README.md";

    sendFile(filepath, &shm_info, &mqfd, &clsem, filenumber);


    filenumber++;
}


