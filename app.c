#include <sys/select.h>

#include "head.h"

#define READ_BUF_AUX_SIZE 128
#define INITIAL_AMOUNT 3
#define FILES_PER_CHILD 5

void waitForView(){
    printf("%s", NAME_SHM);
    sleep(2);
    printf("\n");
}

void redirectPipes(int oldfd, int newfd){
    dup2(oldfd, newfd);
    close(oldfd);
}

void creatingPipes(int pipeWAux[2], int pipeRAux[2], int fdRW[2]){
    if(pipe(pipeWAux) == -1)
    errExit("Error generating pipe_app\n");
    fdRW[1]= pipeWAux[1];

    if(pipe(pipeRAux) == -1)
    errExit("Error generating pipe_chld\n");
    fdRW[0] = pipeRAux[0];
}

void closeAuxPipes(int pipeWAux[2], int pipeRAux[2]){
    close(pipeWAux[0]);
    close(pipeRAux[1]);
}

void closeParallelFD(int child, int fdRW[child][2]){
    for (int i = 0 ; i <= child ; i++) {
        close(fdRW[i][0]);
        close(fdRW[i][1]);
    }
}

void createChild(int pipeWAux[2], int pipeRAux[2], int index, int children_amount, int fdRW[children_amount][2], int *nfds){
    pid_t cpid = fork();

    if(cpid == -1)
    errExit("Error creating child process\n");

    if(cpid == 0){  // child process
        char *newargv[] = {CHILD, NULL};  // passing first files as argument
        char *newenviron[] = {NULL};

        redirectPipes(pipeWAux[0], STDIN_FILENO);  //child reading from stdin
        redirectPipes(pipeRAux[1], STDOUT_FILENO); //child writing to stdout

        closeParallelFD(index, fdRW);

        execve(newargv[0], newargv, newenviron);
        //execve returns on error
        errExit("Execve error\n");
    }
    closeAuxPipes(pipeWAux, pipeRAux);

    if(fdRW[index][0] > *nfds)
        *nfds = fdRW[index][0];
}

int sendChildFile(int fd, int argc, char* argv[], int index, int cant_files){
    if(cant_files == 0 || index == argc) {
        return 0;
    }
    write(fd, argv[index], strlen(argv[index]));
    write(fd, "\n", 1);
    return 1 + sendChildFile(fd, argc, argv, index+1, cant_files-1);
}

void finalClosings(FILE * archivo, struct shmbuf *shmp, int shm_fd){
    fclose(archivo);
    munmap(shmp, sizeof(*shmp));
    close(shm_fd);
    shm_unlink(NAME_SHM);
}

int main(int argc, char* argv[]){
    if(argc == 1)
        errExit("Give at least one file to convert\n");

    setvbuf(stdout,NULL,_IONBF,0);

    int shm_fd;
    struct shmbuf *shmp;

    //-----------------------shm init--------------------------------------
    if((shm_fd = shm_open(NAME_SHM, O_CREAT | O_RDWR | O_TRUNC, 0644)) == -1)
        errExit("Error creating shared memory\n");

    if(ftruncate(shm_fd, sizeof(struct shmbuf)) == -1)
        errExit("Truncate error\n");

    shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,MAP_SHARED, shm_fd, 0);
    if (shmp == MAP_FAILED)
        errExit("Mmap error\n");
    //--------------------------------------------------------------------

    // initializing semaphore in 0
    if (sem_init(&shmp->left_to_read, 1, 0) == -1)
        errExit("Error initializing semaphore read\n");

    int to_read = argc-1;
    shmp->cant_files_to_print = to_read;

    waitForView();

    int children_amount = (to_read)/FILES_PER_CHILD + 1;

    // CHILD*init_amount should be lower than "to_read"
    int init_amount = INITIAL_AMOUNT;
    if(FILES_PER_CHILD < INITIAL_AMOUNT) //to distribute equally (in case, INITIAL_AMOUNT is bigger than FILES_PER_CHILD -> some children will not receive initial amount)
        init_amount= FILES_PER_CHILD;

    int initial_amount_read[children_amount];
    for(int i = 0; i < children_amount ; i++)
        initial_amount_read[i] = init_amount;

    FILE * archivo = fopen("md5file.txt", "w");

    // pipes' variables
    int pipeWAux[2];
    int pipeRAux[2];
    int fdRW[children_amount][2];

    // pipes' validation
    fd_set read_fd_set, read_fd_set_aux;
    FD_ZERO(&read_fd_set);
    FD_ZERO(&read_fd_set_aux);

    int nfds = 0;  //highest numbered file descriptor
    int index = 1; //index of file to be sent to child

    for(int i = 0; i < children_amount; i++){

        creatingPipes(pipeWAux, pipeRAux, fdRW[i]);

        //giving child 2 starting files
        index += sendChildFile(fdRW[i][1], argc, argv, index, init_amount);
        FD_SET(fdRW[i][0], &read_fd_set);  // adding fds to rfds select argument

        createChild(pipeWAux, pipeRAux, i, children_amount, fdRW, &nfds);
    }

    nfds++; // select argument convention

    int retrieved = 0;

    while(retrieved < to_read){
        //Create aux set to execute select
        read_fd_set_aux = read_fd_set;
        int available = select(nfds, &read_fd_set_aux, NULL, NULL, NULL); // ASK (timeout/last argument):  select should 1) specify the timeout duration to wait for event 2) NULL: block indefinitely until one is ready 3) return immediately w/o blocking
        if(available == -1)
            errExit("Select error\n");

        //Check what's available to read
        size_t aux;
        char aux_buff[READ_BUF_AUX_SIZE];
        for(int i = 0; i < children_amount && available != 0; i++) {
            if(FD_ISSET(fdRW[i][0], &read_fd_set_aux) != 0) {

                aux = read(fdRW[i][0], aux_buff, READ_BUF_AUX_SIZE);
                if(aux == -1)
                    errExit("Error reading from pipe\n");
                if (aux == READ_BUF_AUX_SIZE)
                    errExit("Enlarge aux read buffer\n");
                if(shmp->index_of_writing + aux > BUF_SIZE)
                    errExit("No space left on buffer\n");
                aux_buff[aux]=0;

                if(initial_amount_read[i] != 0)
                    initial_amount_read[i]--;

                // Give an additional file to process
                // If there's not left files, close the pipes
                if(initial_amount_read[i] == 0) {
                    int cant_sent = sendChildFile(fdRW[i][1], argc, argv, index, 1);
                    index += cant_sent;
                    if (cant_sent == 0) {
                        close(fdRW[i][0]);
                        close(fdRW[i][1]);
                        FD_CLR(fdRW[i][0], &read_fd_set);
                    }
                }

                //---------------WRITE ON SHM--------------------------
                for (int j = 0; j < aux; ++j)
                    shmp->buf[shmp->index_of_writing + j] = aux_buff[j];
                shmp->index_of_writing += aux;
                shmp->buf[shmp->index_of_writing++]= 0;
                //-----------------------------------------------------

                if(sem_post(&(shmp->left_to_read)) == -1)
                    errExit("Error while posting sem\n");

                fprintf(archivo, "%s\n", aux_buff);

                retrieved++;
                available--;
            }
        }
    }

    finalClosings(archivo, shmp, shm_fd);
    return 0;
}

