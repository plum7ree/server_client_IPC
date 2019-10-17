#define _GNU_SOURCE 
#include <sched.h>
#include <sys/stat.h>
#include <getopt.h>
// #include <pthread.h>
#include "./snappy-c/snappy.h"
#include "./snappy-c/map.h"
#include "./snappy-c/util.h"

#include "TinyFile.h"

#define STACKSIZE 65536
#define ONE_MB 1024*1024

struct clone_arg {
    char msg[MSGSIZE_GLOBAL];
};

struct clone_respond_arg {
    long filesize;
    char* temp_storage;
    char* from_serv_msgbuff;
    int filenumber;
    client_sem_t clsem;
    client_mqfd_t mqfd;
};



sem_t *sem_global;
shm_info_t shm_info;
shm_info_array_t shm_info_array;

void initTinyServer(mqd_t *mqfd);
void initPrivateConnection(int clpid, char *path_mq_to_server, char* path_mq_from_server, \
    char* path_sem_modif, char* path_sem_allow_transf, client_mqfd_t *mqfd, client_sem_t *clsem);
void listenToNewConnection(mqd_t mqfd, char *msgbuff) ;
void connectClient(char* msgbuff) ;
int compressFile(char *input, unsigned long filesize, char **output, unsigned long *outsize) ;
int recvFromClient(char *msgbuff, char **temp_storage, int *fileno, unsigned long *filesz, \
    client_mqfd_t *mqfd, client_sem_t *clsem); 
void sendToClient(int filenumber, int filesize, char *msgbuff, char *temp_storage, \
    client_mqfd_t *mqfd, client_sem_t *clsem );
int clientHandler(void *arg) ;



void initTinyServer(mqd_t *mqfd) {
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


    // *********************new multi seg, size support*********************
    char shmpath[SHM_PATH_SIZE];
    int segsize = shm_info_array.segsize;
    for (int i=0; i<shm_info_array.numseg; i++) {
        snprintf(shmpath, SHM_PATH_SIZE, "%s%d", SHMPATH, i);
        int fd = shm_open(shmpath, O_RDWR| O_CREAT, S_IRUSR | S_IWUSR); /* Open existing object */ 
    

        if (fd == -1)
            perror("shm_open");
        /* Use shared memory object size as length argument for mmap() and as number of bytes to write() */
        struct stat sb;
        if (fstat(fd, &sb) == -1)
            perror("fstat");

        char *mmap_addr = mmap(NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
        // printf("mmap_addr: %x\n", mmap_addr);
        // // fflush(stdout);
        
        if (mmap_addr == MAP_FAILED)
            perror("mmap");

        shm_info_array.array[i].fd = fd;
        shm_info_array.array[i].addr = mmap_addr;
        printf("mmap addr of index %d: %x\n", i, shm_info_array.array[i].addr);

    }

    // *********************previous one segsize ***********************************
    // int fd = shm_open(SHMPATH, O_RDWR| O_CREAT, S_IRUSR | S_IWUSR); /* Open existing object */ 
    

    // if (fd == -1)
    //     perror("shm_open");
    // /* Use shared memory object size as length argument for mmap() and as number of bytes to write() */
    // struct stat sb;
    // if (fstat(fd, &sb) == -1)
    //     perror("fstat");

    // char *mmap_addr = mmap(NULL, SEGSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    // // printf("mmap_addr: %x\n", mmap_addr);
    // // // fflush(stdout);
    
    // if (mmap_addr == MAP_FAILED)
    //     perror("mmap");


    // *addr = mmap_addr;
    // printf("server has been initialized\n");

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

    // tell client server is ready to recieve with just empty msg
    char msgbuff[MSGSIZE_PRIVATE];
    // sending numseg, segsize of shared memory
    createMessage(msgbuff, shm_info_array.numseg, shm_info_array.segsize);
    int status = mq_send(mqfd->mqfd_from_server, msgbuff, MSGSIZE_PRIVATE, 0);
    if (status == -1) {
        perror("sending to client with mqfd_from_serv failure\n");
    }

}


void listenToNewConnection(mqd_t mqfd, char *msgbuff) {
    //mq_receive will block if no element
    int status = mq_receive(mqfd, msgbuff, MSGSIZE_GLOBAL, 0);
    if (status == -1) {
        perror("global mq_receive failure\n");
    }
    printf("recieved message from global queue\n");
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


int compressFile(char *input, unsigned long filesize, char **outbuff, unsigned long *compressed_size) {
    /************************************ Snappy Setting *************************************************/
    struct snappy_env env;
    snappy_init_env(&env);

    int err = snappy_compress(&env, input, (size_t) filesize, *outbuff, (size_t *)compressed_size);       
    if (err) {
        free(*outbuff);
        printf("compression of failed:\n");
        return 0;
    }
    printf("compression successful original: %u new: %u\n", (size_t) filesize, (size_t)  *compressed_size);

    snappy_free_env(&env);

    return 1;
}


int recvFromClient(char *msgbuff, char **temp_store_ptr, int *fileno, unsigned long *filesz, \
    client_mqfd_t *mqfd, client_sem_t *clsem) {

    // recieve first message from client : fist msg :| filenumber(4) | filesize(4)  |
    int status = mq_receive(mqfd->mqfd_to_server, msgbuff, MSGSIZE_PRIVATE, 0);
    if (status == -1) {
        perror("receiving first msg failure\n");
    }

    int filenumber = getFilenumber(msgbuff);
    if (filenumber == DISCONNECT) {
        return DISCONNECT; //disconnect client
    }
    unsigned long filesize = getSizeValue(msgbuff); 
    unsigned long cumsize = 0;
    unsigned long chunksize = 0;
    unsigned long segsize = shm_info_array.segsize;
    int numseg = shm_info_array.numseg;
    unsigned long onechunksize = 0;

    char *temp_storage = malloc(filesize);

    printf("new file request received! filenumber: %d filesize: %lu\n", filenumber, filesize);


    // start read file chunk and store
    while(cumsize < filesize) {
        // real chunk info : | filenumber(4) : int | chunksize(8)  :unsigned long |
        int status = mq_receive(mqfd->mqfd_to_server, msgbuff, MSGSIZE_PRIVATE, 0);
        if (status == -1) {
            perror("receiving file chunk msg failure\n");
        }

        filenumber = getFilenumber(msgbuff); 
        chunksize = getSizeValue(msgbuff); 

        sem_wait(sem_global);
        sem_post(clsem->sem_allow_transf);
        sem_wait(clsem->sem_modif);

        for (int i=0;i<numseg; i++) {
            // NOTE: chunksize is normally "numseg * segsize", onechunksize is normally "segsize"
            //          but we need to handle non-aligned case.
            onechunksize = segsize;
            if(onechunksize > chunksize) { // this means chunsize < segsize, 
                onechunksize = chunksize;
            }
            memcpy(temp_storage + cumsize, shm_info_array.array[i].addr , onechunksize);
            printf("got file! filenumber: %d chunksize: %lu chunkindex: %d cumsize: %lu\n", filenumber, onechunksize, i, cumsize);
            
            cumsize += onechunksize;
            chunksize -= onechunksize;

            if (cumsize >= filesize) {
                break;
            }
        }

    }

    *fileno = filenumber;
    *filesz = filesize;
    *temp_store_ptr = temp_storage;

    return 1;
}

void sendToClient(int filenumber, int filesize, char *msgbuff, char *temp_storage, \
    client_mqfd_t *mqfd, client_sem_t *clsem ) {

    unsigned long cumsize=0;
    unsigned long chunksize=0;
    unsigned long segsize = shm_info_array.segsize;
    int numseg = shm_info_array.numseg;
    unsigned long onechunksize = 0;

    while(cumsize < filesize) {


        chunksize = segsize * numseg; // int * unsigned long ok??
        if (cumsize + chunksize > filesize) {
            chunksize = filesize - cumsize;
        }
        createMessage(msgbuff, filenumber, chunksize);
        // filenumber = *((int *)msgbuff); 
        // chunksize = *((unsigned long *)(msgbuff + HEADER_FNO));
        // printf("checking buffer: file number: %d chunksize: %lu\n", filenumber, chunksize);

        int status = mq_send(mqfd->mqfd_from_server , msgbuff, MSGSIZE_PRIVATE,0);
        if (status == -1) {
            perror("mq_send failure\n");
        }
        printf("mq sent to client\n");
        
        sem_wait(sem_global);
        sem_wait(clsem->sem_allow_transf);

        
        
        for (int i=0;i<numseg; i++) {
            onechunksize = segsize;
            if (onechunksize > chunksize) {
                onechunksize = chunksize;
            }
            memset(shm_info_array.array[i].addr, 0, segsize);
            memcpy(shm_info_array.array[i].addr, temp_storage + cumsize, onechunksize);
            printf("file sent! filenumber: %d chunksize: %lu chunkindex: %d cumsize: %lu\n", filenumber, onechunksize, i, cumsize);
            
            cumsize += onechunksize; 
            chunksize -= onechunksize;

            if(cumsize >= filesize) {
                break;
            }
        }
        
        sem_post(clsem->sem_modif);
        sem_post(sem_global);

        
    }
    printf("end of sentToClient\n");
}


int clientRespondHandler(void *arguments) {
    struct clone_respond_arg *arg = (struct clone_respond_arg*) arguments;
    char *outbuff;
    outbuff = xmalloc(snappy_max_compressed_length((size_t)arg->filesize));
    unsigned long compressed_size = -1 ;
    int res = compressFile(arg->temp_storage, arg->filesize, &outbuff, &compressed_size);

    //********************************SEND BACK TO CLIENT***************************************


    createMessage(arg->from_serv_msgbuff, arg->filenumber, compressed_size);

    int status = mq_send((arg->mqfd).mqfd_from_server, arg->from_serv_msgbuff, MSGSIZE_PRIVATE, 0);
    if (status == -1) {
        perror("sending to client with mqfd_from_serv failure\n");
    }
    printf("mq sent to client\n");


    sendToClient(arg->filenumber, compressed_size, arg->from_serv_msgbuff, arg->temp_storage, &(arg->mqfd), &(arg->clsem));
    printf("FINISHED\n");
    free(outbuff);
}

int clientHandler(void *arg) {
    int clpid = *((int *)(((struct clone_arg *)arg)->msg));
    printf("starting new thread for client pid: %d\n", clpid);
    

    char path_mq_to_server[PRIV_MQ_PATH_SIZE]; // path prefix size should be 4 same as MSGSIZE_GLOBAL
    char path_mq_from_server[PRIV_MQ_PATH_SIZE];
    char path_sem_modif[PRIV_SEM_PATH_SIZE];
    char path_sem_allow_transf[PRIV_SEM_PATH_SIZE];
//    TODO: create a semaphore to lock queue


    client_mqfd_t mqfd;
    client_sem_t clsem;

    initPrivateConnection(clpid, path_mq_to_server, path_mq_from_server, path_sem_modif, \
        path_sem_allow_transf, &mqfd , &clsem);

    int filenumber =-1;
    unsigned long filesize = 0;
    char msgbuff[MSGSIZE_PRIVATE];
    char *temp_storage; 
    char *outbuff;
    char from_serv_msgbuff[MSGSIZE_PRIVATE];

    //************************************recv from client *************************************//
    while(1) {
        
        int conn = recvFromClient(msgbuff, &temp_storage, &filenumber, &filesize, &mqfd, &clsem);
//        TODO: spin new thread to handle compression and send back
        if(conn == DISCONNECT ) {
            printf("disconnecting with client\n");
            break;
        }
        printf("recieve file %d done\n", filenumber);

        struct clone_respond_arg *respondArg;
//        TODO: free this
        respondArg = malloc(sizeof(struct clone_respond_arg));
        respondArg->clsem = clsem;
        respondArg->filenumber = filenumber;
        respondArg->filesize = filesize;
        respondArg->from_serv_msgbuff = from_serv_msgbuff;
        respondArg->mqfd = mqfd;
        respondArg->temp_storage = temp_storage;

        pthread_t cThread;
        pthread_create(&cThread, NULL, clientRespondHandler, respondArg);
        continue;
        //***********************************compress*************************************************
        outbuff = xmalloc(snappy_max_compressed_length((size_t)filesize));
        unsigned long compressed_size = -1 ;
        int res = compressFile(temp_storage, filesize, &outbuff, &compressed_size);

        //********************************SEND BACK TO CLIENT***************************************
        
        
        createMessage(from_serv_msgbuff, filenumber, compressed_size);

        int status = mq_send(mqfd.mqfd_from_server, from_serv_msgbuff, MSGSIZE_PRIVATE, 0);
        if (status == -1) {
            perror("sending to client with mqfd_from_serv failure\n");
        }
        printf("mq sent to client\n");


        sendToClient(filenumber, compressed_size, from_serv_msgbuff, temp_storage, &mqfd, &clsem);

    }
    
    //******************************************************************************************* 
    free(outbuff);
    free(temp_storage);


    return 0 ;
}






int
main(int argc, char *argv[])
{
    
    // ./server --n_sms 5 --sms_size 32
    
    int numseg;
    int segsize;

    int c;
    int digit_optind = 0;

    static struct option long_options[] = {
            {"n_sms",     required_argument, 0,  0 },
            {"sms_size",  required_argument, 0,  0 },
            {0,         0,                 0,  0 }
        };

    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        

        c = getopt_long(argc, argv, "", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 0:
                if (!strcmp(long_options[option_index].name, "n_sms")) {
                    numseg = atoi(optarg);
                } 
                else if (!strcmp(long_options[option_index].name, "sms_size")) {
                    segsize = atoi(optarg);
                }
                break;

            case '?':
                printf("wrong arguments. exiting program\n");
                exit(EXIT_SUCCESS);
                break;

            default:
                break;
        }
    }

    if (argc < 5) {
        printf("too few args bro!!\n ex) ./server --n_sms 1 --sms_size 32\n");
        exit(0);
    }

    shm_info_t arr[numseg];
    shm_info_array.numseg = numseg;
    shm_info_array.segsize = segsize;
    shm_info_array.array = arr;


    client_mqfd_t mqfd;

    initTinyServer(&mqfd.mqfd_global);

    while(1) {
        //1. check mq for new client creation
        char msgbuff[MSGSIZE_GLOBAL];
        listenToNewConnection(mqfd.mqfd_global, msgbuff);
        //2. create thread for that client (clone)
        connectClient(msgbuff);


    }




}
