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

#define SHMPATH "/shm_obj1" // naming convention: /somename. Not ./somename

#define SEMPATH_GLOBAL "/sem_global"
#define SEMPATH "/sem_obj1"
#define SEMPATH2 "/sem_obj2"
#define SEMPATH3 "/sem_obj3"

#define MQPATH_GLOBAL "/mq_global"

#define SEGSIZE 10
#define MSGSIZE_GLOBAL 4
#define MSGSIZE_PRIVATE 4
#define MAXMSGNUM 10

#define PRIV_MSG_PATH_SIZE 8
#define PRIV_SEM_PATH_SIZE 8

#define MQ_TO_SERVER_INDEX 0
    #define MQ_FROM_SERVER_INDEX 1
    #define SEM_MODIF_INDEX 0
    #define SEM_ALLOW_TRANSF_INDEX 1

/**packet: 
Header : | method(1byte) | PID (4byte) | FILENAME_int (4byte) | SIZE(4byte) ) | => 13byte

**/
/** Methods:
FILE (char value: 1) | PID (4byte) | FILENAME_int (4byte) | TOTALSIZE(4byte) | no content
FILE2(char value: 2) | PID (4byte) | FILENAME_int (4byte) | size of fraction(4byte) | content

**/

#define MHD_FILE  '1'
#define MHD_FILE2 '2'

#define HEADER_SIZE 13


// char *filepath = "peda-session-client.txt";

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
    int len;
} shm_info_t;



size_t fwrite_buf( void const* ptr, size_t size, FILE* stream);
size_t fread_buf( void* ptr, size_t size, FILE* stream);


