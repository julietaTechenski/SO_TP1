#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>  // for shm
#include <sys/select.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

struct shmbuf {
    sem_t mutex;
    size_t cnt;
};

// included in select.h
//struct timespec{
//    time_t tv_sec;
//    long tv_nsec;
//};

int main(int argc, char* argv[]){
    int children_amount = (argc-1)/10;

    char *shmpath = "shm.txt";
    int shmfd;
    struct shmbuf *shmp;

    if((shmfd = shm_open(shmpath, O_CREAT | O_RDWR | O_TRUNC, 0644)) == -1){
        perror("Error creating shared memory\n");
        exit(EXIT_FAILURE);
    }

    shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,MAP_SHARED, shmfd, 0);
//    if (shmp == MAP_FAILED)
//        perror("mmap\n");
//        exit(EXIT_FAILURE);

    // initializing semaphore in 0
    if (sem_init(&shmp->mutex, 1, 0) == -1) {
        perror("Error initializing semaphre\n");
        exit(EXIT_FAILURE);
    }

    // waiting for the view process
    if(sem_timedwait(&shmp->mutex, ) == -1){
        perror("Error while waiting for the view process to start\n");
        exit(EXIT_FAILURE);
    }

    // pipes' variables
    int pipefd[children_amount][2];
    pid_t cpid[children_amount];
    // pipes' validation

    fd_set rfds;
    FD_ZERO(&rfds);
    int nfds = 0;  //highest numbered file descriptor


    for(int i = 0; i < children_amount; i++) {
        if(pipe(pipefd[i]) == -1){
            perror("Error generating pipe\n");
            exit(EXIT_FAILURE);
        }
        FD_SET(pipefd[0][i], &rfds);  // adding fds to rfds select argument
    }



    int to_read = argc - 1; // files to be read counter

    for(int i = 0; i < children_amount; i++){

        cpid[i] = fork();
        if(cpid[i] < 0){
            perror("Error creating child process\n");
            exit(EXIT_FAILURE);
        }
        if(cpid[i] == 0){  // child process
            char *newargv[] = {"child", argv[i], NULL};  // passing first files as argument
            char *newenviron[] = {NULL};

            execve(newargv[0], newargv,newenviron);
            exit(EXIT_SUCCESS);
        }
        if(nfds < cpid[i] || i == 0)
            nfds = cpid[i];
    }

    nfds++; // select argument convention

    while(to_read > 0){
        int available = select(nfds, &rfds, NULL, NULL, NULL); // ASK (timeout/last argument):  select should 1) specify the timeout duration to wait for event 2) NULL: block indefinitely until one is ready 3) return immediately w/o blocking
        for(int i = 0; i < children_amount ; i++) {
            if(FD_ISSET(cpid[i][0],&rfds) == 1) {
                /*reads*/
                /*writes to shared memory*/
                available--;
            }
        }
        to_read--;
    }

    shm_unlink(shmpath);

    return 0;
}
