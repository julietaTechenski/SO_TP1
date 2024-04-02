#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>  // for shm
#include <sys/select.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>

#define CHILD "./child"

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
    int to_read = argc-1;
    int children_amount = (to_read)/10 + 1;

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
    pid_t cpid;
    // pipes' validation

    fd_set rfds;
    FD_ZERO(&rfds);

    int nfds = 1;  //highest numbered file descriptor
    int index = 1;

    for(int i = 0; i < children_amount; i++){

        //creating pipe between app and child
        if(pipe(pipefd[i]) == -1){
            perror("Error generating pipe\n");
            exit(EXIT_FAILURE);
        }
        FD_SET(pipefd[i][0], &rfds);  // adding fds to rfds select argument
        write(pipefd[i][1], argv[index], strlen(argv[index])); //giving 1 file to convert
        index++;

        //-------------------------------------------------------------------

        cpid = fork();

        if(cpid == -1){
            perror("Error creating child process\n");
            exit(EXIT_FAILURE);
        }

        if(cpid == 0){  // child process
            char *newargv[] = {CHILD, NULL};  // passing first files as argument
            char *newenviron[] = {NULL};

            dup2(pipefd[i][0], 0);
            dup2(pipefd[i][1], 1);
            for (int j = 0; j < i; ++j) {       //closing fd with other child except mine
                close(pipefd[j][0]);
                close(pipefd[j][1]);
            }

            execve(newargv[0], newargv, newenviron);
            //execve returns on error
            perror("Execve error\n");
            exit(EXIT_FAILURE);
        }

        if(nfds < cpid)
            nfds = cpid;
    }

    nfds++; // select argument convention

    fd_set read_set_aux;
    int retrieved = 0;
    while(retrieved < to_read){
        read_set_aux = rfds;
        int available = select(nfds, &read_set_aux, NULL, NULL, NULL); // ASK (timeout/last argument):  select should 1) specify the timeout duration to wait for event 2) NULL: block indefinitely until one is ready 3) return immediately w/o blocking
        if(available==-1){
            perror("Select error\n");
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i < children_amount && available != 0; i++) {
            if(FD_ISSET(pipefd[i][0], &read_set_aux) != 0) {

                //TODO read and copy to the shm
                retrieved++;

                if(index<argc) {
                    write(pipefd[i][1], argv[index], strlen(argv[index]));
                    index++;
                }
                available--;
            }
        }
    }

    shm_unlink(shmpath);

    return 0;
}
