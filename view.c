// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <signal.h>

#include "head.h"

#define MAX_PATH_LENGTH 128

int main(int argc, char* argv[]){
    char shmpath[MAX_PATH_LENGTH+1];

    if(argc == 1) {
        ssize_t aux;
        if((aux = read(STDIN_FILENO,shmpath, MAX_PATH_LENGTH)) == ERROR) {
            errExit("Error in read while getting shmpath parameter\n");
        }
        shmpath[aux] = '\0';  // null terminated
    } else if(argc == 2) {
        strncpy(shmpath, argv[1], sizeof(shmpath) - 1);
        shmpath[sizeof(shmpath) - 1] = '\0';
    } else {
        errExit("Error incorrect amount of parameters for view.\n Try running with with a pipe or a shm path as parameter\n");
    }

    //Opening shared memory object
    int fd;

    if((fd = shm_open(shmpath, O_RDWR, 0644)) == ERROR) {
        errExit("Error in shm_open function while accessing shared memory\n");
    }

    //Mapping shared memory into the caller's address space
    struct shmbuf * shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shmp == MAP_FAILED) {
        errExit("Error in mmap function. shm could not be mapped from view\n");
    }

    shmp->index_of_reading = 0; //Initializing index to zero

    printf("FILE - MD5_HASH - SLAVE_PID\n");    //Format of output

    while(shmp->cant_files_to_print){

        if(sem_wait(&shmp->left_to_read) == ERROR) {   //Waits for something to read
            errExit("Error in sem_wait function\n");
        }

        //Prints output
        int aux = printf("%s", &shmp->buf[shmp->index_of_reading]);

        shmp->index_of_reading += aux; //Amount read without \n
        shmp->index_of_reading++; //Jumping null
        printf("\n");

        shmp->cant_files_to_print--;
    }

    sem_destroy(&shmp->left_to_read);
    munmap(shmp, sizeof(*shmp));
    close(fd);
    return 0;
}