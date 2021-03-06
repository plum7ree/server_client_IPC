#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>
#include <mqueue.h>

#define SHMPATH "/shm_obj" // naming convention: /somename. Not ./somename
#define SHM_PATH_SIZE 20


#define SEMPATH_GLOBAL "/sem_global"
#define SEMPATH "/sem_obj1"
#define SEMPATH2 "/sem_obj2"
#define SEMPATH3 "/sem_obj3"

#define MQPATH_GLOBAL "/mq_global"


// #define SEGSIZE 10
#define MSGSIZE_GLOBAL 4
#define MSGSIZE_PRIVATE 12
#define MAXMSGNUM 10


// file names
#define FILENAMESIZE 50


//******** SEM, MQ PATH****************************************

#define PATH_PID_SIZE 8
#define PATH_INDEX_SIZE 3

#define PRIV_MQ_PATH_SIZE (PATH_PID_SIZE + PATH_INDEX_SIZE + 1 + 1)  // ex) if pid: 3131, index:1, path: /00003131001
#define PRIV_SEM_PATH_SIZE (PATH_PID_SIZE + PATH_INDEX_SIZE + 1 + 1) //

#define MQ_TO_SERVER_INDEX 0
#define MQ_FROM_SERVER_INDEX 1
#define SEM_MODIF_INDEX 10
#define SEM_ALLOW_TRANSF_INDEX 11
//*************************************************************


//*****protocol***********************************************
#define HEADER_FNO 4
#define HEADER_CHUNK 8
#define HEADER_FSZ 8

#define MAX_STORAGE 2
#define DISCONNECT (-1)
#define SYNC (-2)
#define MAX_FILES 200
//*******************************************************


// link with -lrt -pthread
typedef struct mqd_t_client {
    mqd_t mqfd_global;
    mqd_t mqfd_to_server;
    mqd_t mqfd_from_server;
} client_mqfd_t;

typedef struct client_sem {
    sem_t *sem_modif;
    sem_t *sem_allow_transf;
    // sem_t *sem_allow_to_server_msg ;
    // sem_t *sem_to_server_sent ;
    // sem_t *sem_allow_from_server_msg;
    // sem_t *sem_from_server_sent;
} client_sem_t;

typedef struct shm_info {
    int fd;
    char *addr;
} shm_info_t;

typedef struct shm_info_array{
    int numseg;
    unsigned long segsize;
    shm_info_t *array;
} shm_info_array_t ;

typedef struct cst{
    struct timeval tv1;
    struct timeval tv2;
    float interval;
} cst_t;





size_t fwrite_buf( void const* ptr, size_t size, FILE* stream);
size_t fread_buf( void* ptr, size_t size, FILE* stream);
void createMessage(char *msgbuff, int fileno, unsigned long size);

int getFilenumber(char *msgbuff);

unsigned long getSizeValue(char *msgbuff);



