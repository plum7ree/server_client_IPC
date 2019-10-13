#define _GNU_SOURCE 
#include <sched.h>
#include <sys/stat.h>
#include "./snappy-c/snappy.h"
#include "./snappy-c/map.h"
#include "./snappy-c/util.h"

#include "TinyFile.h"

#define STACKSIZE 1024
#define ONE_MB 1024*1024

struct clone_arg {
    char msg[MSGSIZE_GLOBAL];
};



sem_t *sem_global;
shm_info_t shm_info;





int compressFile(char *input, int filesize, char **output) {
    /************************************ Snappy Setting *************************************************/
    char *outbuffer = xmalloc(snappy_max_compressed_length(filesize));
    size_t outlen = snappy_max_compressed_length(filesize);
    struct snappy_env env;
    snappy_init_env(&env);

    int err = snappy_compress(&env, input, (size_t) filesize, outbuffer, &outlen);       
    if (err) {
        free(outbuffer);
        printf("compression of failed:\n");
        return 0;
    }

    *output = outbuffer;
    snappy_free_env(&env);

    return 1;
}

void sendFileToClient(int filenumber, int filesize, char *msgbuff, char *temp_storage, \
    client_mqfd_t *mqfd, client_sem_t *clsem ) {

    unsigned long cumsize=0;
    unsigned long  chunksize=0;

    while(cumsize < filesize) {


        unsigned long chunksize = SEGSIZE;
        if (cumsize + chunksize > filesize) {
            chunksize = filenumber - cumsize;
        }
        createMessage(msgbuff, filenumber, chunksize);
        // filenumber = *((int *)msgbuff); 
        // chunksize = *((unsigned long *)(msgbuff + HEADER_FNO));
        // printf("checking buffer: file number: %d chunksize: %lu\n", filenumber, chunksize);

        int status = mq_send(mqfd->mqfd_from_server , msgbuff, MSGSIZE_PRIVATE,0);
        if (status == -1) {
            perror("mq_send failure\n");
        }
        

        sem_wait(clsem->sem_allow_transf);
        memset(shm_info.addr, 0, SEGSIZE);
        memcpy(shm_info.addr, temp_storage + cumsize, chunksize);
        printf("file sent! filenumber: %d chunksize: %lu cumsize: %lu\n", filenumber, chunksize, cumsize);
        sem_post(clsem->sem_modif);

        cumsize += chunksize; 
    }
}

void recvFromClient(char *msgbuff, char *temp_storage, int *fileno, unsigned long *filesz, \
    client_mqfd_t *mqfd, client_sem_t *clsem) {


    // tell client server is ready to recieve with just empty msg
    int status = mq_send(mqfd->mqfd_from_server, msgbuff, MSGSIZE_PRIVATE, 0);
    if (status == -1) {
        perror("sending to client with mqfd_from_serv failure\n");
    }

    // recieve first message from client : fist msg :| filenumber(4) | filesize(4)  |
    status = mq_receive(mqfd->mqfd_to_server, msgbuff, MSGSIZE_PRIVATE, 0);
    if (status == -1) {
        perror("receiving first msg failure\n");
    }

    int filenumber = getFilenumber(msgbuff);
    unsigned long filesize = getSizeValue(msgbuff); 
    unsigned long cumsize = 0;
    unsigned long chunksize = 0;
    printf("new file request recved! filenumber: %d filesize: %lu\n", filenumber, filesize); 


    // start read file chunk and store
    while(cumsize < filesize) {
        // real chunk info : | filenumber(4) : int | chunksize(8)  :unsigned long |
        int status = mq_receive(mqfd->mqfd_to_server, msgbuff, MSGSIZE_PRIVATE, 0);
        if (status == -1) {
            perror("receiving file chunk msg failure\n");
        }

        filenumber = getFilenumber(msgbuff); 
        chunksize = getSizeValue(msgbuff); 
        printf("got msg chunk! filenumber: %d chunksize: %lu\n", filenumber, chunksize);

        sem_wait(sem_global);
        sem_post(clsem->sem_allow_transf);
        sem_wait(clsem->sem_modif);


        memcpy(temp_storage + cumsize, shm_info.addr, chunksize);
        printf("got file! filenumber: %d chunksize: %lu\n", filenumber, chunksize);
        cumsize += chunksize;

        sem_post(sem_global);

    }


    *fileno = filenumber;
    *filesz = filesize;


}


void initPrivateConnection(int clpid, char *path_mq_to_server, char* path_mq_from_server, \
    char* path_sem_modif, char* path_sem_allow_transf, client_mqfd_t *mqfd, client_sem_t *clsem) {


    snprintf(path_mq_to_server, PRIV_MQ_PATH_SIZE,"/%0*d%0*d", (int)PATH_PID_SIZE, clpid, (int)PATH_INDEX_SIZE, (int)MQ_TO_SERVER_INDEX);// ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//
    snprintf(path_mq_from_server, PRIV_MQ_PATH_SIZE, "/%0*d%0*d", (int)PATH_PID_SIZE, clpid, (int)PATH_INDEX_SIZE, (int)MQ_FROM_SERVER_INDEX);// ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//
    snprintf(path_sem_modif,PRIV_SEM_PATH_SIZE, "/%0*d%0*d", (int)PATH_PID_SIZE, clpid, (int)PATH_INDEX_SIZE, (int)SEM_MODIF_INDEX);// ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//
    snprintf(path_sem_allow_transf,PRIV_SEM_PATH_SIZE, "/%0*d%0*d", (int)PATH_PID_SIZE, clpid, (int)PATH_INDEX_SIZE, (int)SEM_ALLOW_TRANSF_INDEX);// ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//


    printf("path_mq_to_server: %s\n", path_mq_to_server);
    printf("path_mq_from_server: %s\n", path_mq_from_server);
    printf("path_sem_modif: %s\n", path_sem_modif);
    printf("path_sem_allow_transf: %s\n", path_sem_allow_transf);

    mqd_t mqfd_to_serv = mq_open(path_mq_to_server, O_RDWR);
    if(mqfd_to_serv == -1) {
        perror("mqfd_to_serv mq_open failure");
        exit(0);
    }
    mqd_t mqfd_from_serv = mq_open(path_mq_from_server, O_RDWR);
    if(mqfd_from_serv == -1) {
        perror("mqfd_from_serv mq_open failure");
        exit(0);
    }
    sem_t *sem_modif = sem_open(path_sem_modif , O_RDWR);
    if(sem_modif == SEM_FAILED) {
        perror("sem_modif sem_open failed\n");
        exit(0);
    }
    sem_t *sem_allow_transf = sem_open(path_sem_allow_transf , O_RDWR);
    if(sem_allow_transf == SEM_FAILED) {
        perror("sem_allow_transf sem_open failed\n");
        exit(0);
    }
    mqfd->mqfd_to_server = mqfd_to_serv;
    mqfd->mqfd_from_server =  mqfd_from_serv;
    clsem->sem_allow_transf = sem_allow_transf;
    clsem->sem_modif = sem_modif;


}


int clientHandler(void *arg) {
    int clpid = *((int *)(((struct clone_arg *)arg)->msg));
    printf("starting new thread for client pid: %d\n", clpid);
    

    char path_mq_to_server[PRIV_MQ_PATH_SIZE]; // path prefix size should be 4 same as MSGSIZE_GLOBAL
    char path_mq_from_server[PRIV_MQ_PATH_SIZE];
    char path_sem_modif[PRIV_SEM_PATH_SIZE];
    char path_sem_allow_transf[PRIV_SEM_PATH_SIZE];


    client_mqfd_t mqfd;
    client_sem_t clsem;

    initPrivateConnection(clpid, path_mq_to_server, path_mq_from_server, path_sem_modif, \
        path_sem_allow_transf, &mqfd , &clsem);

    printf("2\n");

    printf("path_mq_to_server: %s\n", path_mq_to_server);
    printf("path_mq_from_server: %s\n", path_mq_from_server);
    printf("path_sem_modif: %s\n", path_sem_modif);
    printf("path_sem_allow_transf: %s\n", path_sem_allow_transf);

    printf("2\n");
    int filenumber =-1;
    unsigned long filesize = 0;

    //************************************recv from client *************************************//

    char msgbuff[MSGSIZE_PRIVATE];
    char *temp_storage = malloc(filesize); 
    recvFromClient(msgbuff, temp_storage, &filenumber, &filesize, &mqfd, &clsem);



    //***********************************compress*************************************************
    char *outbuff;
    compressFile(temp_storage, filesize, &outbuff);

    printf("compression successful!\n");




    //********************************SEND BACK TO CLIENT***************************************
    sem_init(clsem.sem_allow_transf, 0, 0);
    sem_init(clsem.sem_modif, 0, 0);
    char from_serv_msgbuff[MSGSIZE_PRIVATE];
    createMessage(from_serv_msgbuff, filenumber, filesize);
    int status = mq_send(mqfd.mqfd_from_server, msgbuff, MSGSIZE_PRIVATE, 0);
    if (status == -1) {
        perror("sending to client with mqfd_from_serv failure\n");
    }

    sendFileToClient(filenumber, filesize, from_serv_msgbuff, temp_storage, &mqfd, &clsem);

    //******************************************************************************************* 
    free(outbuff);
    free(temp_storage);


    return 0 ;
}


void connectClient(char* msgbuff) {
    // arg
    // 1. copy msgbuff (pid) name
    
    char *child_stack = malloc(STACKSIZE);
    child_stack += STACKSIZE; //stack top, stack grows down
    struct clone_arg *arg = (struct clone_arg *) malloc(sizeof(struct clone_arg));
    memcpy(arg->msg, msgbuff, MSGSIZE_GLOBAL);
    int clone_pid = clone(clientHandler, (void *)child_stack, SIGCHLD, (void *)arg);
}



void initTinyServer(mqd_t *mqfd, char **addr) {
    struct mq_attr attr;
    attr.mq_maxmsg = MAXMSGNUM;
    attr.mq_msgsize = MSGSIZE_GLOBAL;
    attr.mq_flags = 0;
    *mqfd = mq_open(MQPATH_GLOBAL, O_RDWR | O_CREAT, 0666, &attr);

    if(*mqfd == -1) {
        perror("Parent mq_open failure");
        exit(0);
    }


    sem_global = sem_open(SEMPATH_GLOBAL , O_RDWR|O_CREAT, S_IRUSR | S_IWUSR, 1);
    if(sem_global == SEM_FAILED) {
        perror("sem global failed\n");
        exit(0);
    }
    sem_init(sem_global, 0, 1); 
    int val;
    sem_getvalue(sem_global, &val);
    printf("sem_global init vale: %d\n", val);

    int fd = shm_open(SHMPATH, O_RDWR| O_CREAT, S_IRUSR | S_IWUSR); /* Open existing object */ 
    

    if (fd == -1)
        perror("shm_open");
    /* Use shared memory object size as length argument for mmap() and as number of bytes to write() */
    struct stat sb;
    if (fstat(fd, &sb) == -1)
        perror("fstat");

    char *mmap_addr = mmap(NULL, SEGSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    // printf("mmap_addr: %x\n", mmap_addr);
    // // fflush(stdout);
    
    if (mmap_addr == MAP_FAILED)
        perror("mmap");


    *addr = mmap_addr;
    printf("server has been initialized\n");

}

void listenToNewConnection(mqd_t mqfd, char *msgbuff) {
    //mq_receive will block if no element
    int status = mq_receive(mqfd, msgbuff, MSGSIZE_GLOBAL, 0);
    if (status == -1) {
        perror("global mq_receive failure\n");
    }
    printf("recieved message from global queue\n");
}



int
main(int argc, char *argv[])
{
    
    client_mqfd_t mqfd;

    initTinyServer(&mqfd.mqfd_global, &shm_info.addr);

    while(1) {
        //1. check mq for new client creation
        char msgbuff[MSGSIZE_GLOBAL];
        listenToNewConnection(mqfd.mqfd_global, msgbuff);
        //2. create thread for that client (clone)
        connectClient(msgbuff);
    }




}
