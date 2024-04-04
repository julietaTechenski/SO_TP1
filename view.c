#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#define BUF_SIZE 1024

void shm_unlink_handler(int sig);

volatile sig_atomic_t shutdown_flag = 0;

void shm_unlink_handler(int sig) {
    if(sig == SIGINT){
        shutdown_flag = 1;
    }
}

struct shmbuf {
    sem_t  sem_mutex;            /* POSIX unnamed semaphore */
    sem_t  sem_read;            /* POSIX unnamed semaphore */
    size_t index;             /* Index of reading */
    size_t cnt;             /* Number of bytes used in 'buf' */
    char buf[BUF_SIZE];   /* Data being transferred */
};

/*
 * stdin as parameter: pathname of a shared memory object and the string that is to be copied
 * */
int main(int argc, char* argv[]){
    // creating signal handler for when shm is unlinked
    signal(SIGINT, shm_unlink_handler);

    if(argc != 3){
        perror("Incorrect parameters passed to view\n");
        exit(EXIT_FAILURE);
    }

    char *shmpath = argv[1];

    // opening shm object
    int fd = shm_open(shmpath, O_RDONLY, 0644);
    if(fd == -1){
       perror("View couldn't open the shm\n");
       exit(EXIT_FAILURE);
    }

    //mapping shm into the caller's address space
    struct shmbuf *shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shmp == MAP_FAILED){
        perror("shm couldn't be mapped\n");
        exit(EXIT_FAILURE);
    }

    shmp->index = 0; // initializing index to zero

    while(!shutdown_flag){
        if(sem_wait(&shmp->sem_read) == -1){   // waits to be sth to read
            perror("Error in semaphores in view\n");
            exit(EXIT_FAILURE);
        }

        if(sem_wait(&shmp->sem_mutex) == -1){   // waits to have access to the shm
            perror("Error in semaphores in view\n");
            exit(EXIT_FAILURE);
        }

        int i;
        for(i = shmp->index; shmp->buf[i] != '\0'; i++)
            printf("%s", shmp->buf[i]);

        printf("\n");
        shmp->index+=i;

        if(sem_post(&shmp->sem_post) == -1){
            perror("Error in semaphores in view\n");
            exit(EXIT_FAILURE);
        }

        if(sem_post(&shmp->sem_mutex) == -1){   // waits to have access to the shm
            perror("Error in semaphores in view\n");
            exit(EXIT_FAILURE);
        }

    }
    return 0;
}