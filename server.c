
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

void clientHandler(void *arg) {
    char *msg = ((struct clone_arg *)arg)->msg;
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
