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

int clientHandler(void *arg) {
    char *pid = ((struct clone_arg *)arg)->msg;

    char path_mq_to_server[MSGSIZE_GLOBAL]; // path prefix size should be 4 same as MSGSIZE_GLOBAL
    char path_mq_from_server[MSGSIZE_GLOBAL];
    char path_sem_modif[MSGSIZE_GLOBAL];
    char path_sem_allow_transf[MSGSIZE_GLOBAL];

    sprintf(path_mq_to_server, "/%s%03d", pid, MQ_TO_SERVER_INDEX);
    sprintf(path_mq_from_server,"/%s%03d", pid, MQ_FROM_SERVER_INDEX);
    sprintf(path_sem_modif,"/%s%03d", pid, SEM_MODIF_INDEX);
    sprintf(path_sem_allow_transf,"/%s%03d", pid, SEM_ALLOW_TRANSF_INDEX);

    mqd_t mqfd_to_serv = mq_open(path_mq_to_server, O_RDWR);
    mqd_t mqfd_from_serv = mq_open(path_mq_from_server, O_RDWR);
    sem_t *sem_modif = sem_open(path_sem_modif , O_RDWR);
    sem_t *sem_allow_transf = sem_open(path_sem_allow_transf , O_RDWR);


    int filenumber;
    int filesize;
    int chunksize = 0;
    int cumsize = 0;
    char filechunk[SEGSIZE];

    // recieve first message : fist msg :| filenumber(4) | filesize(4)  |
    char msgbuff[MSGSIZE_PRIVATE];
    int status = mq_receive(mqfd_to_serv, msgbuff, MSGSIZE_PRIVATE, 0);
    if (status == -1) {
        perror("receiving first msg failure\n");
    }

    char header_fno[HEADER_FNO];
    memcpy(header_fno, msgbuff, HEADER_FNO);
    filenumber = atoi(header_fno); 

    char header_fsz[HEADER_FNO];
    memcpy(header_fsz, msgbuff + HEADER_FNO, HEADER_FSZ);
    filesize = atoi(header_fsz); 

    char *temp_storage = malloc(filesize); 


    // start read file chunk and store
    while(cumsize < filesize) {
        // real chunk info : | filenumber(4) | chunksize(4) |
        int status = mq_receive(mqfd_to_serv, msgbuff, MSGSIZE_PRIVATE, 0);
        if (status == -1) {
            perror("receiving file chunk msg failure\n");
        }

        memcpy(header_fno, msgbuff, HEADER_FNO);
        filenumber = atoi(header_fno); 

        char header_chunk[HEADER_FSZ];
        memcpy(header_chunk, msgbuff + HEADER_FNO, HEADER_CHUNK);
        chunksize = atoi(header_chunk);


        sem_wait(sem_global);
        sem_post(sem_allow_transf);
        sem_wait(sem_modif);


        memcpy(filechunk + cumsize, shm_info.addr, chunksize);

        cumsize += chunksize;

        sem_post(sem_global);

    }
    
    char *outbuff;
    compressFile(temp_storage, filesize, &outbuff);



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
    printf("starting new thread for client pid: %s\n", msgbuff);

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


    sem_global = sem_open(SEMPATH_GLOBAL , O_RDWR|O_CREAT, S_IRUSR | S_IWUSR, 0);
    if(sem_global == SEM_FAILED) {
        perror("sem global failed\n");
        exit(0);
    }

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
