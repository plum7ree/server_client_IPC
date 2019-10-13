
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

// int searchFile(int filenumber) {
//     for(int i = 0; i < MAX_STORAGE; i++) {
//         if(file_info_array[i].filenumber == filenumber) {
//             return 1;
//         }
//     }

//     return 0;
// }

// int createNewFileInfo(int filenumber, int filesize) {
//     // recieve first message : fist msg :| filenumber(4) | filesize(4)  |


//     // create storage buffer in memory, don't forget to free later!!!!
//     file_info_t *file_info = malloc(sizeof(file_info_t));
//     file_info->filenumber = filenumber;
//     file_info->filesize = filesize;
//     file_info->temp_storage = malloc(filesize); 

//     file_info_array

//     return 1;

// }

void clientHandler(void *arg) {
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


    // never ends until client is sending done.
    while(1) {
        // real chunk info : | filenumber(4) | chunksize(4) |
        int status = mq_receive(mqfd_to_serv, msgbuff, MSGSIZE_PRIVATE, 0);
        if (status == -1) {
            perror("receiving first msg failure\n");
        }

        char header_fno[HEADER_FNO];
        memcpy(header_fno, msgbuff, HEADER_FNO);
        filenumber = atoi(header_fno); 

        // // if this is new file, create storage in memory for this file.
        // if(!searchFile(filenumber)) {

        //     char header_chunk[HEADER_FSZ];
        //     memcpy(header_chunk, msgbuff + HEADER_FNO, HEADER_FSZ);
        //     filesize = atoi(header_chunk);

        //     if (!createNewFileInfo(filenumber, filesize)) { // there are limited number of storage for file requests.
        //         perror("maximum number of file for storage exceeds!\n"); // client should handle maximum number of file for async
        //     }
        //     continue;
        // }

        char header_chunk[HEADER_CHUNK];
        memcpy(header_chunk, msgbuff + HEADER_FNO, HEADER_FSZ);
        chunksize = msgbuff



    }
    




    free(file_info->temp_storage);
    free(file_info);


//     sem_allow_transf = // sem only shared between one client and server

    
//     while()
//     {
//         sem_wait(sem_global); // shared with all clients

//         // 1. read request in message queue (file name, chunk size)
//         sem_post(sem_allow_msg)
//         sem_wait(sem_msg_sent)
//         msg = readMsg()
//         filenumber
//         chunksize = CHUNKSIZE(msg);

//         if(file_info_array[filenumber].filesize == 0) {
//             // if filenumber not exist in file_info_array:
//             //  first packet includes filenumber, chunksize, filesize
//             //  further packet includes filenumber, chunksize
//             file_info_array[filenumber].filesize = FILESIZE(msg);
//             file_info_array[filenumber].cumsize = 0;
//             chunksize = CHUNKSIZE(msg);
//             char *buffer = malloc(FILESIZE(msg));
//             file_info_array[filenumber].buffer = buffer;
//         }
//         sem_post(sem_msg_sent)


//         sem_post(sem_allow_transf)
//         sem_wait(sem_modif);
//         char *temp_file_buffer =  file_info_array[filenumber].buffer + cumSize

//         memcopy(temp_file_buffer, mmap_addr, chunksize);


//         file_info_array[filenumber].cumsize += chunksize;
//         totalsize = file_info_array[filenumber].filesize;

//         if (cumSize >= totalsize) {
//             break;
//         }

//     }

//     compress();
    

//     sem_wait(sem_global); // server should acquire this to write to shared memory





//     return;
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

    // int status = mq_receive(mqfd, msgbuff, 1000, 0);
    // if (status == -1) {
    //     perror("mq_receive failure\n");
    // }

    // long fileSize = (long) atoi(msgbuff);
    // printf("filesize: %lu\n", fileSize);

    // mq_close(mqfd);
    // mq_unlink(MQPATH);

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
