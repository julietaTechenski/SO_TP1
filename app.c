#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>  // for shm
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define CHILD "./child"
#define BUF_SIZE 1024
#define NAME_SHM "/shm.txt"

#define errExit(msg)    {perror(msg); exit(EXIT_FAILURE);}

struct shmbuf {
    sem_t  sem;            /* POSIX unnamed semaphore */
    size_t cnt;             /* Number of bytes used in 'buf' */
    char buf[BUF_SIZE];   /* Data being transferred */
};

int main(int argc, char* argv[]){
    int to_read = argc-1;
    int children_amount = (to_read)/10 + 1;

    int flag=0;
    int shm_fd;
    struct shmbuf * shmp;

    if((shm_fd = shm_open(NAME_SHM, O_CREAT | O_RDWR | O_TRUNC, 0644)) == -1)
        errExit("Error creating shared memory\n");

    if(ftruncate(shm_fd, sizeof(struct shmbuf)) == -1)
        errExit("Truncate error\n");

    printf(NAME_SHM);
    shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,MAP_SHARED, shm_fd, 0);
    if (shmp == MAP_FAILED)
        errExit("Mmap error\n");

    // initializing semaphore in 0
    if (sem_init(&shmp->sem, 1, 0) == -1)
        errExit("Error initializing semaphre\n");

    // waiting for the view process
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        errExit("Clock error\n");
    ts.tv_sec += 2;

    if(!(sem_timedwait(&(shmp->sem), &ts) == -1 && errno == ETIMEDOUT))
        flag=1;

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

            dup2(pipefd[i][0], STDOUT_FILENO);
            dup2(pipefd[i][1], STDIN_FILENO);
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

        int aux=0;
        char aux_buff[128];
        for(int i = 0; i < children_amount && available != 0; i++) {
            if(FD_ISSET(pipefd[i][0], &read_set_aux) != 0) {
                aux = read(pipefd[i][0], aux_buff, 128);
                if(index<argc) {
                    write(pipefd[i][1], argv[index], strlen(argv[index]));
                    index++;
                }
                if(flag){
                    //post to shared mem and post
                }
                retrieved++;
                available--;
            }
        }
    }

    shm_unlink(NAME_SHM);

    return 0;
}

