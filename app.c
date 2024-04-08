#include <sys/select.h>

#include "head.h"

#define READ_BUF_AUX_SIZE 64

int main(int argc, char* argv[]){
    if(argc == 1)
        errExit("Give at least one file to convert\n");

    int to_read = argc-1;
    int children_amount = (to_read)/5 + 1;

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

    printf("%s", NAME_SHM);
    sleep(5);

    // pipes' variables
    int pipeWAux[2];
    int fdW[children_amount];
    int pipeRAux[2];
    int fdR[children_amount];
    pid_t cpid;

    // pipes' validation

    fd_set rfds, read_set_aux;
    FD_ZERO(&rfds);
    FD_ZERO(&read_set_aux);

    int nfds = 0;  //highest numbered file descriptor
    int index = 1; //index of file to be sent to child

    for(int i = 0; i < children_amount; i++){
        //creating pipe between app and child
        if(pipe(pipeWAux) == -1)
            errExit("Error generating pipe_app\n");
        fdW[i]= pipeWAux[1];

        if(pipe(pipeRAux) == -1)
            errExit("Error generating pipe_chld\n");
        fdR[i] = pipeRAux[0];

        write(fdW[i], argv[index], strlen(argv[index]) + 1); //giving 1 file to convert
        index++;
        FD_SET(fdR[i], &rfds);  // adding fds to rfds select argument

        //-------------------------------------------------------------------

        cpid = fork();

        if(cpid == -1)
            errExit("Error creating child process\n");

        if(cpid == 0){  // child process
            char *newargv[] = {CHILD, NULL};  // passing first files as argument
            char *newenviron[] = {NULL};

            dup2(pipeWAux[0], STDIN_FILENO);  //child reading from stdin
            close(pipeWAux[0]);   //child reading pipe

            dup2(pipeRAux[1], STDOUT_FILENO); //child writing to stdout
            close(pipeRAux[1]);   //child writing pipe

            for (int j = 0; j <= i; ++j) {       //closing fd with other child except mine
                close(fdW[j]);
                close(fdR[j]);
            }

            execve(newargv[0], newargv, newenviron);
            //execve returns on error
            errExit("Execve error\n");
        }

        close(pipeWAux[0]);
        close(pipeRAux[1]);

        if(fdR[i] > nfds)
            nfds = fdR[i];
    }

    nfds++; // select argument convention

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
            if(FD_ISSET(fdR[i], &read_set_aux) != 0) {

                aux = read(fdR[i], aux_buff, READ_BUF_AUX_SIZE);
                if(aux == -1)
                    errExit("Error reading from pipe\n");

                if(shmp->cnt + aux > BUF_SIZE)
                    errExit("No space left on buffer\n");

                // Give an additional file to process
                // If there's not left files, close the pipes
                if(index<argc) {
                    write(fdW[i], argv[index], strlen(argv[index]));
                    index++;
                }else{
                    close(fdW[i]);
                    close(fdR[i]);
                    FD_CLR(fdR[i], &rfds);
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

    //shm_unlink(NAME_SHM);
    return 0;
}

