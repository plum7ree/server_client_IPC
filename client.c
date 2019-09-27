#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>

#include "tinyipc.h"


// link with -lrt -pthread

int
main(int argc, char *argv[])
{   
    int r;
    int fd;
    size_t len = LEN;
    char *addr;
    char *buff = malloc(len);
    /* Size of shared memory object */
    sem_t *sem, *sem_modif;

    sem = sem_open(SEMPATH , O_RDWR,0);
    sem_modif = sem_open(SEMPATH2 , O_RDWR,S_IRUSR | S_IWUSR);

    if (sem == -1)
        perror("sem_open");

    fd = shm_open(SHMPATH, O_RDWR , 0);
    if (fd == -1)
        perror("shm_open");

    if (ftruncate(fd, len) == -1)
        perror("ftruncate failed");
                                                /* Open existing object */
    addr = mmap(NULL, len, PROT_WRITE, MAP_SHARED, fd, 0); 
    if (addr == MAP_FAILED)
            perror("mmap");
    

    printf("mmap addr: %x", addr);
    fflush(stdout); // printf doesn't print exactly when is was called.

    // int value = -2;
    // sem_getvalue(sem_modif, &value);
    // printf("sem val: %d", value);


    while(1) {
        sem_wait(sem);
        scanf("%s", buff);
        printf("start to copying: %s\n", buff);
        fflush(stdout);
        // buff = "a";
        memcpy(addr, buff, len); /* Copy string to shared memory */ 
        printf("string copied:");
        fflush(stdout);
        r = sem_post(sem_modif);
        if (r==-1)
            perror("sem post sem_modif");

    }


    
    munmap(addr, len);
    shm_unlink(SHMPATH);
    if (close(fd) == -1)
        perror("close"); /* 'fd' is no longer needed */
    exit(EXIT_SUCCESS);
}