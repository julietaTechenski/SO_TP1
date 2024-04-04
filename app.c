#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>  // for shm
#include <sys/select.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>

#define CHILD "./child"
#define BUF_SIZE 1024
#define READ_BUF_AUX_SIZE 64
#define NAME_SHM "/app_shm"

#define errExit(msg) {perror(msg); exit(EXIT_FAILURE);}

struct shmbuf {
    sem_t  sem_mutex;            /* POSIX unnamed semaphore */
    sem_t  sem_read;            /* POSIX unnamed semaphore */
    size_t index;             /* Index of reading */
    size_t cnt;             /* Number of bytes used in 'buf' */
    char buf[BUF_SIZE];   /* Data being transferred */
};

int main(int argc, char* argv[]){
    if(argc == 1)
        errExit("Give at least one file to convert\n");

    int to_read = argc-1;
    int children_amount = (to_read)/10 + 1;

    int shm_fd;
    struct shmbuf * shmp;

    if((shm_fd = shm_open(NAME_SHM, O_CREAT | O_RDWR | O_TRUNC, 0644)) == -1)
        errExit("Error creating shared memory\n");

    if(ftruncate(shm_fd, sizeof(struct shmbuf)) == -1)
        errExit("Truncate error\n");

    shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,MAP_SHARED, shm_fd, 0);
    if (shmp == MAP_FAILED)
        errExit("Mmap error\n");

    // initializing semaphore in 0
    if (sem_init(&shmp->sem_read, 1, 0) == -1)
        errExit("Error initializing semaphore read\n");

    if (sem_init(&shmp->sem_mutex, 1, 1) == -1)
        errExit("Error initializing semaphore mutex\n");

    sleep(5);

    // pipes' variables
    int pipefd[children_amount][2];
    pid_t cpid;

    // pipes' validation

    fd_set rfds;
    FD_ZERO(&rfds);

    int nfds = 1;  //highest numbered file descriptor
    int index = 1; //index of file to be sent to child

    for(int i = 0; i < children_amount; i++){

        //creating pipe between app and child
        if(pipe(pipefd[i]) == -1)
            errExit("Error generating pipe\n");

        write(pipefd[i][1], argv[index], strlen(argv[index])); //giving 1 file to convert
        index++;
        FD_SET(pipefd[i][0], &rfds);  // adding fds to rfds select argument

        //-------------------------------------------------------------------

        cpid = fork();

        if(cpid == -1)
            errExit("Error creating child process\n");

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
            errExit("Execve error\n");
        }

        if(nfds < cpid)
            nfds = cpid;
    }

    nfds++; // select argument convention

    fd_set read_set_aux;
    int retrieved = 0;

    while(retrieved < to_read){
        //Create aux set to execute select
        read_set_aux = rfds;
        int available = select(nfds, &read_set_aux, NULL, NULL, NULL); // ASK (timeout/last argument):  select should 1) specify the timeout duration to wait for event 2) NULL: block indefinitely until one is ready 3) return immediately w/o blocking
        if(available == -1)
            errExit("Select error\n");

        //Check what's available to read
        size_t aux;
        char aux_buff[READ_BUF_AUX_SIZE];
        for(int i = 0; i < children_amount && available != 0; i++) {
            if(FD_ISSET(pipefd[i][0], &read_set_aux) != 0) {
                aux = read(pipefd[i][0], aux_buff, READ_BUF_AUX_SIZE);
                if(shmp->cnt+aux > BUF_SIZE)
                    errExit("No space left on buffer\n");

                // Give an additional file to process
                // If there's not left files, close the pipes
                if(index<argc) {
                    write(pipefd[i][1], argv[index], strlen(argv[index]));
                    index++;
                }else{
                    close(pipefd[i][0]);
                    close(pipefd[i][1]);
                }

                if(sem_wait(&(shmp->sem_mutex)) == -1)
                    errExit("Error while waiting to write\n");

                //---------------WRITE ON SHM--------------------------
                for(int j = 0; j < aux; ++j)
                    shmp->buf[shmp->cnt + j] = aux_buff[j];
                shmp->cnt += aux;
                shmp->buf[shmp->cnt++]= 0;
                //-----------------------------------------------------

                if(sem_post(&(shmp->sem_mutex)) == -1)
                    errExit("Error while posting sem\n");

                if(sem_post(&(shmp->sem_read)) == -1)
                    errExit("Error while posting sem\n");

                retrieved++;
                available--;
            }
        }
    }

    shm_unlink(NAME_SHM);
    return 0;
}

