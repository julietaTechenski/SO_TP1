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
    sem_t  sem_read;            /* POSIX unnamed semaphore */
    sem_t sem_write;
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
    char *string = argv[2];
    size_t len = strlen(string);

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


    while(!shutdown_flag){
        sem_wait(&sem_read);
        if(sem_wait(&shmp->sem) == -1){
            perror("Error waiting for semaphores in view\n");
            exit(EXIT_FAILURE);
        }

        printf("%s", &shmp->buf);

        sem_post(&sem_write);
    }

    return 0;
}