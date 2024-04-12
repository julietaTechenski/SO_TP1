#include <signal.h>

#include "head.h"

#define MAX_PATH_LENGTH 128

int main(int argc, char* argv[]){

    char shmpath[MAX_PATH_LENGTH];
    if(argc == 1) {
        read(STDIN_FILENO,shmpath, MAX_PATH_LENGTH);
    } else if(argc == 2) {
        strcpy(shmpath, argv[1]);
    } else {
        errExit("Error incorrect amount of parameters for view.\n Try running with with a pipe or a shm path as parameter\n");
    }

    // opening shm object

    int fd;

    if((fd = shm_open(shmpath, O_RDWR, 0644)) == ERROR) {
        errExit("Error in shm_open function while accessing shared memory\n");
    }

    //mapping shm into the caller's address space
    struct shmbuf * shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shmp == MAP_FAILED) {
        errExit("Error in mmap function. shm could not be mapped from view\n");
    }

    shmp->index_of_reading = 0; // initializing index to zero

    printf("FILE - MD5_HASH - SLAVE_PID\n");
    while(shmp->cant_files_to_print){

        if(sem_wait(&shmp->left_to_read) == ERROR) {   // waits to be sth to read
            errExit("Error in sem_wait function\n");
        }

        int aux = printf("%s", &shmp->buf[shmp->index_of_reading]);
        shmp->index_of_reading += aux; //car read - \n
        shmp->index_of_reading++; //salir del null
        printf("\n");

        shmp->cant_files_to_print--;
    }

    sem_destroy(&shmp->left_to_read);
    munmap(shmp, sizeof(*shmp));
    close(fd);
    return 0;
}