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


int
main(int argc, char *argv[])
{
    int fd;
    int len = LEN;
    char *addr;
    struct stat sb;
    sem_t *sem, *sem_modif;

    sem = sem_open(SEMPATH , O_RDWR| O_CREAT,S_IRUSR | S_IWUSR);
    sem_modif = sem_open(SEMPATH2 , O_RDWR| O_CREAT,S_IRUSR | S_IWUSR);


    sem_init(sem, 1, 1);
    sem_init(sem_modif, 1, 0);

    fd = shm_open(SHMPATH, O_RDWR| O_CREAT, S_IRUSR | S_IWUSR); /* Open existing object */ 
    printf("fd: %d\n", fd);
    fflush(stdout);

    if (fd == -1)
        perror("shm_open");
    /* Use shared memory object size as length argument for mmap() and as number of bytes to write() */
    if (fstat(fd, &sb) == -1)
        perror("fstat");

    addr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    printf("mmap addr: %x\n", addr);
    fflush(stdout);

    if (addr == MAP_FAILED)
        perror("mmap");
    


    

    while(1) {
        sem_wait(sem_modif);
        write(STDOUT_FILENO, addr, len); 
        printf("\n");
        fflush(stdout);
        sem_post(sem);
    }


    if (close(fd) == -1);
        perror("close");
    exit(EXIT_SUCCESS);
}
/* 'fd' is no longer needed */