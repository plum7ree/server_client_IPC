#include "yaml.h"
#include <assert.h>

#include "TinyFile.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>

shm_info_array_t shm_info_array; 
cst_t cst[MAX_FILES];
int sync_mode = 0;


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

int initPrivateMessageQueue(mqd_t *mqfd_ptr, int index, int non_blocking) {
    int pid = getpid();
    char msgbuff[PRIV_MQ_PATH_SIZE];

    snprintf(msgbuff,PRIV_MQ_PATH_SIZE ,"/%0*d%0*d", (int)PATH_PID_SIZE, pid, (int)PATH_INDEX_SIZE, index);// ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//
    // path will look like /00007442001 if pid 7442, index 1
    // index 0 : msg queue from cleint to the server, 
    // index 1 : msg queue from the server to client

    struct mq_attr attr;
    attr.mq_maxmsg = MAXMSGNUM;
    attr.mq_msgsize = MSGSIZE_PRIVATE;
    attr.mq_flags = 0;
    mqd_t fd;
    if (non_blocking) {
        fd = mq_open(msgbuff, O_RDWR | O_CREAT | O_NONBLOCK, 0666, &attr);
    } else {
        fd = mq_open(msgbuff, O_RDWR | O_CREAT, 0666, &attr);
    }

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
    // path will look like /00007442001 if pid 7442, index 10
    sem_t *sem = sem_open(msgbuff , O_RDWR | O_CREAT ,S_IRUSR | S_IWUSR, 0);
    // index 0 : sem_allow_transf, 
    // index 1 : sem_modif


    if (sem == SEM_FAILED){
        perror("sem_open");
    }
    printf("sem path: %s\n", msgbuff);
    *sem_ptr = sem;
}


void initSharedMem(unsigned long segsize, int index) {
    char shmpath[SHM_PATH_SIZE];
    snprintf(shmpath, SHM_PATH_SIZE, "%s%d", SHMPATH, index);
    int fd = shm_open(shmpath, O_RDWR , 0);
    if (fd == -1)
        perror("shm_open");
    if (ftruncate(fd, segsize) == -1)
        perror("ftruncate failed");
                                                /* Open existing object */
    char *addr = mmap(NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    if (addr == MAP_FAILED)
        perror("mmap");
    

    printf("mmap addr: %x\n", addr);
    fflush(stdout); // printf doesn't print exactly when is was called.
    shm_info_array.array[index].fd = fd;
    shm_info_array.array[index].addr = addr;

}


void initClient(client_sem_t *clsem, client_mqfd_t *mqfd) {
    initGlobalMessageQueue(&mqfd->mqfd_global);
    //meta data of the file (how many chunks)
    // fist msg :| filenumber(4) | filesize(8)  |
    // other msg:| filenumber(4) | chunksize(8) |
    initPrivateMessageQueue(&mqfd->mqfd_to_server, MQ_TO_SERVER_INDEX, 0);
    //TODO: remove 
    //2 semaphores per client
    initPrivateMessageQueue(&mqfd->mqfd_from_server, MQ_FROM_SERVER_INDEX, 1);

    /* NOTE: NOT SURE WHAT THIS IS */
    /* shm_info->len = SEGSIZE; */
    /* initSharedMem(&shm_info->fd, &shm_info->addr, shm_info->len); */
    initSemaphore(&clsem->sem_modif, SEM_MODIF_INDEX);
    initSemaphore(&clsem->sem_allow_transf, SEM_ALLOW_TRANSF_INDEX);

}





void connectToServer(client_mqfd_t *mqfd, client_sem_t *clsem) {
    int pid = getpid();
    char msgbuff[MSGSIZE_GLOBAL];
    *(int*)msgbuff = pid; // we are sending integer, not string!
    int status = mq_send(mqfd->mqfd_global, msgbuff, MSGSIZE_GLOBAL, 0);
    if(status == -1){
        perror("global mq_send failed\n");
        exit(0);
    }
    printf("message with pid:%d has been sent to global queue\n",pid);


    char msgbuff_from_server[MSGSIZE_PRIVATE];
    // wait until server responsd. IF we do not have this, server's mq_open(mqfd_to_server) happens later than file transfer.
    // and server doesn't respond at all.
    /* mq_receive(mqfd->mqfd_from_server,msgbuff_from_server ,MSGSIZE_PRIVATE,0); */
    while (mq_receive(mqfd->mqfd_from_server,msgbuff_from_server ,MSGSIZE_PRIVATE,0) == -1);
    int numseg = getFilenumber(msgbuff_from_server);
    unsigned long segsize = getSizeValue(msgbuff_from_server);
    shm_info_array.numseg = numseg;
    shm_info_array.segsize = segsize;
    shm_info_t *arr = (shm_info_t *) malloc(sizeof(shm_info_t) * numseg);
    shm_info_array.array = arr;

    for (int i=0; i< numseg; i++) {
        initSharedMem(segsize, i);
    }

}

void notifyDoneSending(client_mqfd_t *mqfd) {
    char msgbuff[MSGSIZE_PRIVATE];
    createMessage(msgbuff, DISCONNECT, DISCONNECT);
    int status = mq_send(mqfd->mqfd_to_server, msgbuff, MSGSIZE_PRIVATE,0);
    if (status == -1) {
        perror("mq_send failure\n");
    }
    printf("Sent DONE signal!\n");
}

void notifySyncMode(client_mqfd_t *mqfd) {
    char msgbuff[MSGSIZE_PRIVATE];
    createMessage(msgbuff, SYNC, SYNC);
    int status = mq_send(mqfd->mqfd_to_server, msgbuff, MSGSIZE_PRIVATE,0);
    if (status == -1) {
        perror("mq_send failure\n");
    }
    printf("Sent SYNC signal!\n");
}
void sendFile(char *path, client_mqfd_t *mqfd, client_sem_t *clsem, int filenumber) {

    char msgbuff[MSGSIZE_PRIVATE];
    FILE *pFile;
    unsigned long cumsize = 0;
    unsigned long segsize = shm_info_array.segsize;
    unsigned long onechunksize =0;
    int numseg = shm_info_array.numseg;
    char chunkbuff[segsize * numseg];

    printf("OPEN %s \n", path);
    pFile = fopen(path, "rb");
    if (pFile==NULL) {
        perror("file open error!\n");
    }

    // obtain file size:
    fseek (pFile , 0 , SEEK_END);
    unsigned long filesize = ftell (pFile);
    sprintf(chunkbuff, "%lu", filesize);
    rewind (pFile);
    // printf("file size: %lu\n", filesize);


    // protocol : | filenumber (4bytes) | filesize (8) |
    createMessage(msgbuff, filenumber, filesize);

    int status = mq_send(mqfd->mqfd_to_server , msgbuff, MSGSIZE_PRIVATE,0);
    if (status == -1) {
        perror("mq_send failure\n");
    }
    // printf("first msg sent!\n");

    while(cumsize < filesize) {

        unsigned long chunksize = fread_buf(chunkbuff, segsize * numseg, pFile);
        createMessage(msgbuff, filenumber, chunksize);
        // filenumber = *((int *)msgbuff); 
        // chunksize = *((unsigned long *)(msgbuff + HEADER_FNO));
        // printf("checking buffer: file number: %d chunksize: %lu\n", filenumber, chunksize);

        int status = mq_send(mqfd->mqfd_to_server , msgbuff, MSGSIZE_PRIVATE,0);
        if (status == -1) {
            perror("mq_send failure\n");
        }
        // printf("msg sent to server, chunksize: %lu\n", chunksize);

        sem_wait(clsem->sem_allow_transf);

        for (int i=0;i<numseg; i++) {
            onechunksize = segsize;
            if (onechunksize > chunksize) {
                onechunksize = chunksize;
            }
            memset(shm_info_array.array[i].addr, 0, segsize);
            memcpy(shm_info_array.array[i].addr, chunkbuff + i*segsize, onechunksize); // cumsize must be zero when i =0!
            // printf("file sent! filenumber: %d chunksize: %lu chunkindex: %d cumsize: %lu\n", filenumber, onechunksize, i, cumsize);

            cumsize += onechunksize;
            chunksize -= onechunksize;

            if(cumsize >= filesize) {
                break;
            }
        }

        sem_post(clsem->sem_modif);

    }
    fclose(pFile);

}

// return 1 if received a file, else 0
int recvFile(client_mqfd_t *mqfd, client_sem_t *clsem, char fname_array[MAX_FILES][FILENAMESIZE]) {

    int filenumber;
    int status;
    unsigned long filesize = 0;
    unsigned long cumsize = 0;
    unsigned long chunksize = 0;
    unsigned long onechunksize = 0;
    int numseg = shm_info_array.numseg;
    unsigned int segsize = shm_info_array.segsize;
    FILE *pFile;
    char msgbuff[MSGSIZE_PRIVATE];

    // recieve first message from client : fist msg :| filenumber(4) | filesize(8)  |
    // printf("wait to receive compressed file!\n");
    do {
        status = mq_receive(mqfd->mqfd_from_server, msgbuff, MSGSIZE_PRIVATE, 0);
        if (!sync_mode && (status == -1)) {
            // printf("GIVING BACK CONTROL TO MAIN THREAD\n");
            return 0;
        }
    } while (status == -1);
//    if (status == -1) {
//        perror("receiving first msg failure\n");
//    }

    filenumber = getFilenumber(msgbuff);
    filesize = getSizeValue(msgbuff); 
    // printf("new file request recved! filenumber: %d filesize: %lu\n", filenumber, filesize);
    pFile = fopen(fname_array[filenumber], "wb+");
    // char *temp_storage = malloc(filesize);  


    // start read file chunk and store
    while(cumsize < filesize) {
        // real chunk info : | filenumber(4) : int | chunksize(8)  :unsigned long |
        do {
            status = mq_receive(mqfd->mqfd_from_server, msgbuff, MSGSIZE_PRIVATE, 0);
        } while(status == -1);

        filenumber = getFilenumber(msgbuff); 
        chunksize = getSizeValue(msgbuff); 
        // printf("got msg chunk! filenumber: %d chunksize: %lu\n", filenumber, chunksize);

        sem_post(clsem->sem_allow_transf);
        sem_wait(clsem->sem_modif); 

        for (int i=0;i<numseg; i++) {
            // NOTE: chunksize is normally "numseg * segsize", onechunksize is normally "segsize"
            //          but we need to handle non-aligned case.
            onechunksize = segsize;
            if(onechunksize > chunksize) { // this means chunsize < segsize, 
                onechunksize = chunksize;
            }

            fwrite_buf(shm_info_array.array[i].addr, onechunksize, pFile);


            // printf("got file! filenumber: %d chunksize: %lu chunkindex: %d cumsize: %lu\n", filenumber, onechunksize, i, cumsize);
            cumsize += onechunksize;
            chunksize -= onechunksize;

            // printf("got file! filenumber: %d chunksize: %lu, total: %lu\n", filenumber, chunksize, cumsize);
            if (cumsize >= filesize) {
                break;
            }
        }


    }
    // printf("Done receiving file %d: %d/%d\n", filenumber, cumsize, filesize);
    fclose(pFile);



    gettimeofday(&cst[filenumber].tv2, NULL);
    cst[filenumber].interval = (float)(cst[filenumber].tv2.tv_sec - cst[filenumber].tv1.tv_sec)*1000000 + \
                               (float)(cst[filenumber].tv2.tv_usec - cst[filenumber].tv1.tv_usec);


    return 1;
}



char *trim(char *s) {
    char *ptr;
    if (!s)
        return NULL;   // handle NULL string
    if (!*s)
        return s;      // handle empty string
    for (ptr = s + strlen(s) - 1; (ptr >= s) && isspace(*ptr); --ptr);
    ptr[1] = '\0';
    return s;
}
int parseYAML(char* path, char files[][FILENAMESIZE]) {

    FILE *file;
    yaml_parser_t parser;
    yaml_event_t event;
    int done = 0;
    int count = 0;
    int error = 0;
    char* value;
    int expecting_filename = 0;

    printf("Parsing '%s':\n", path);
    fflush(stdout);

    file = fopen(path, "rb");
    assert(file);

    assert(yaml_parser_initialize(&parser));

    yaml_parser_set_input_file(&parser, file);
    int i = 0;
    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            error = 1;
            break;
        }

        if (event.type == YAML_SCALAR_EVENT) {
            value = event.data.scalar.value;
            if (expecting_filename) {
                sprintf(files[i++], "%s", trim(value));
                expecting_filename = 0;
            }
            if (strcmp(value, "FILE_TYPE") == 0) {
                expecting_filename = 1;
            }
        }
        done = (event.type == YAML_STREAM_END_EVENT);

    }
    yaml_event_delete(&event);
    assert(!fclose(file));
    return i;
}

void writeCST(cst_t *cst, int fileCount, unsigned long segsize) {
    int nfileType = 5;


    if(fileCount <= nfileType) {
        //******************* SIMPLE TEST****************************
        char csvfile[60];
        char *fmt = "../output/simple.csv";
        snprintf(csvfile, 60, fmt, shm_info_array.numseg);

        FILE *pfile = fopen(csvfile, "a+");
        char filetype[5][10] = {"tiny","small","medium","large","huge"};

        float sum = 0;
        // float cst_avg = 0;
        for(int i=0; i<fileCount; i++) {
            sum = cst[i].interval;
            fprintf(pfile, "%f,%s\n", sum, filetype[i]);
        }
        // sum = sum / 1000000;

        // cst_avg = sum / (float) fileCount;
        
        
        fclose(pfile);

    } else {
        //********************* STRESS TEST****************************
        int nfile_per_type = 20;

        float sum = 0;
        char csvfile[60];
        char *fmt;
        if (sync_mode){
            fmt = "../output/stress_testing_sync_%d_segment.csv";
        } else {
            fmt = "../output/stress_testing_async_%d_segment.csv";
        }
        
        snprintf(csvfile, 60, fmt, shm_info_array.numseg);

        FILE *pfile = fopen(csvfile, "a+");


        // float cst_avg = 0;
        for(int i=0; i<(fileCount/nfile_per_type); i++) {
            for (int j=0; j < nfile_per_type; j++) {
                sum += cst[(i*nfile_per_type + j)].interval;
            }
            sum = sum / 1000000;
            fprintf(pfile, "%.2f,%lu\n", sum, segsize);
            sum = 0;
        }
        

        // cst_avg = sum / (float) fileCount;
        
        
        fclose(pfile);
    }


}

void freeEverything(client_mqfd_t *mqfd, client_sem_t *clsem) {

    /******************** 1. free mq **********************/
    mqd_t mqfd_global;
    mqd_t mqfd_to_server;
    mqd_t mqfd_from_server;
    mq_close(mqfd->mqfd_global);
    mq_close(mqfd->mqfd_to_server);
    mq_close(mqfd->mqfd_from_server);

    // DO NOT mq_unlink(mqfd_global)
    int pid = getpid();
    char msgbuff[PRIV_MQ_PATH_SIZE];

    snprintf(msgbuff,PRIV_MQ_PATH_SIZE ,"/%0*d%0*d", (int)PATH_PID_SIZE, pid, (int)PATH_INDEX_SIZE, MQ_TO_SERVER_INDEX);// ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//
    mq_unlink(msgbuff);
    snprintf(msgbuff,PRIV_MQ_PATH_SIZE ,"/%0*d%0*d", (int)PATH_PID_SIZE, pid, (int)PATH_INDEX_SIZE, MQ_FROM_SERVER_INDEX);// ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//
    mq_unlink(msgbuff);
    
    /******************** 2. free sem **********************/
    sem_close(clsem->sem_modif);
    sem_close(clsem->sem_allow_transf);
    snprintf(msgbuff,PRIV_SEM_PATH_SIZE ,"/%0*d%0*d", (int)PATH_PID_SIZE, pid, (int)PATH_INDEX_SIZE, SEM_MODIF_INDEX);// ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//
    sem_unlink(msgbuff);
    snprintf(msgbuff,PRIV_SEM_PATH_SIZE ,"/%0*d%0*d", (int)PATH_PID_SIZE, pid, (int)PATH_INDEX_SIZE, SEM_ALLOW_TRANSF_INDEX);// ***IMPORTANT PRIV_MSG_PATH_SIZE : 8 ******//
    sem_unlink(msgbuff);
    

    /******************** 3. free shm **********************/
    char shmpath[SHM_PATH_SIZE];

    for (int index=0; index<shm_info_array.numseg; index++) {
        // DO NOT SHM_UNLINK , do only close and munmap
        close(shm_info_array.array[index].fd);
        munmap(shm_info_array.array[index].addr, shm_info_array.segsize);
    }
    free(shm_info_array.array);
    // mq_unlink(MQPATH);

}

void disConnect(client_mqfd_t *mqfd) {
    char msgbuff[MSGSIZE_PRIVATE];
    createMessage(msgbuff, -1, 0);
    mq_send(mqfd->mqfd_to_server, msgbuff, MSGSIZE_PRIVATE, 0);

}

int
main(int argc, char *argv[])
{   

    if (argc < 3) {
        printf("too few args bro!!\n ex) ./client --conf ../input/sample.yaml\n");
        exit(0);
    }

    client_mqfd_t mqfd;

    client_sem_t clsem;

    char fname_array[MAX_FILES][FILENAMESIZE];
    int fileCount;
    int filenumber = 0;
    char filepath[FILENAMESIZE];
    for (int i=0; i <argc; i++) {
        if (strcmp(argv[i], "--conf") == 0) {
            fileCount = parseYAML(argv[++i], fname_array);
        }
        if (strcmp(argv[i], "--state") == 0) {
            if (strcmp(argv[++i], "SYNC") == 0) {
                sync_mode = 1;
            }
        }
    }

    initClient(&clsem, &mqfd);
    connectToServer(&mqfd, &clsem);

    // ********************* cst setting*****************************//

    if (sync_mode) {
        notifySyncMode(&mqfd);
    }

    for(filenumber=0; filenumber < fileCount; filenumber++) {
        snprintf(filepath, FILENAMESIZE, "%s", fname_array[filenumber]);
        printf("%s\n", fname_array[filenumber]);

        gettimeofday(&cst[filenumber].tv1, NULL);

        sendFile(filepath, &mqfd, &clsem, filenumber);
        snprintf(fname_array[filenumber], FILENAMESIZE,"../output/%s.compressed",filepath);
        if (sync_mode) {
            recvFile(&mqfd, &clsem, fname_array);
        }


    }
    notifyDoneSending(&mqfd);
    while((!sync_mode) && filenumber) {
        filenumber -= recvFile(&mqfd, &clsem, fname_array);
    };

    /* disConnect(&mqfd); */

    writeCST(cst, fileCount,shm_info_array.segsize);

    freeEverything(&mqfd, &clsem);

    printf("CLIENT DONE\n");

}


