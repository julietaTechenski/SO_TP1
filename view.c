#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#include "head.h"

void shm_unlink_handler(int sig);

volatile sig_atomic_t shutdown_flag = 0;

void shm_unlink_handler(int sig) {
    if(sig == SIGINT){
        shutdown_flag = 1;
    }
}

/*
 * stdin as parameter: pathname of a shared memory object and the string that is to be copied
 * */
int main(int argc, char* argv[]){
    // creating signal handler for when shm is unlinked
    signal(SIGINT, shm_unlink_handler);

    char shmpath[MAX_PATH_LENGTH];
    if(!isatty(fileno(stdin))){
        fgets(shmpath, MAX_PATH_LENGTH, stdin);
    }else{
        fgets(shmpath, MAX_PATH_LENGTH, stdout);
        printf("%s\n", shmpath);
    }

    // opening shm object

    int fd;

    if((fd = shm_open(NAME_SHM, O_CREAT | O_RDWR | O_TRUNC, 0644)) == -1)
    errExit("Error creating shared memory\n");

    if(ftruncate(fd, sizeof(struct shmbuf)) == -1)
    errExit("Truncate error\n");

    //mapping shm into the caller's address space
    struct shmbuf * shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shmp == MAP_FAILED)
        errExit("shm couldn't be mapped\n");

    shmp->index = 0; // initializing index to zero

    while(!shutdown_flag){
        if(sem_wait(&shmp->sem_read) == -1)   // waits to be sth to read
            errExit("Error in semaphores in view\n");

        if(sem_wait(&shmp->sem_mutex) == -1)   // waits to have access to the shm
            errExit("Error in semaphores in view\n");

        int aux = printf("%s", &shmp->buf[shmp->index]);
        printf("\n");
        shmp->index += aux;
        shmp->index++; //salir del null

        if(sem_post(&shmp->sem_mutex) == -1)  // waits to have access to the shm
            errExit("Error in semaphores in view\n");
    }

    close(fd);
    shm_unlink(NAME_SHM);
    return 0;
}