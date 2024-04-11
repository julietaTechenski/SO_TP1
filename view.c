#include <signal.h>

#include "head.h"

int main(int argc, char* argv[]){

    char shmpath[MAX_PATH_LENGTH];
    if(argc == 1) {
        fgets(shmpath, MAX_PATH_LENGTH, stdin);
        shmpath[strlen(shmpath) - 1] = 0; //reading \n
    }else if(argc == 2)
        strcpy(shmpath,argv[1]);
    else
        errExit("Incorrect amount of parameters for view, try running with with a pipe or a shm path as parameter\n");

    // opening shm object

    int fd;

    if((fd = shm_open(shmpath, O_RDWR, 0644)) == -1)
      errExit("Error accessing shared memory\n");

    //mapping shm into the caller's address space
    struct shmbuf * shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shmp == MAP_FAILED)
        errExit("shm couldn't be mapped\n");

    shmp->index_of_reading = 0; // initializing index to zero

    while(!shmp->app_done_writing || shmp->index_of_writing != shmp->index_of_reading){
        if(sem_wait(&shmp->left_to_read) == -1)   // waits to be sth to read
            errExit("Error in semaphores in view\n");

        int aux = printf("%s", &shmp->buf[shmp->index_of_reading]);
        shmp->index_of_reading += aux;
        shmp->index_of_reading++; //salir del null
    }

    sem_destroy(&shmp->left_to_read);
    munmap(shmp, sizeof(*shmp));
    close(fd);
    return 0;
}