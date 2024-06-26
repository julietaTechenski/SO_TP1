// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <sys/select.h>

#include "head.h"

#define FILES_PER_CHILD 5


int main(int argc, char* argv[]){
    if(argc == 1){
        errExit("Error incorrect amount of parameters. Give at least one file to convert\n");
    }

    setvbuf(stdout,NULL,_IONBF,0);

    //Shared memory variable
    int shm_fd;
    struct shmbuf * shmp = initializeShm(&shm_fd);

    //Variables for files to read
    int to_read = argc-1;
    shmp->cant_files_to_print = to_read;

    waitForView();

    int children_amount = (to_read)/FILES_PER_CHILD + 1;
    int initial_amount_read[children_amount];
    int init_amount = manageInitialAmountOfFiles(children_amount, initial_amount_read, to_read);

    //Output file
    FILE * file;
    if((file = fopen(FILE_NAME, WRITING)) == NULL){
        errExit("Error in fopen while opening md5_output.txt file\n");
    }

    //Pipes' variables
    int pipe_w_aux[2];
    int pipe_r_aux[2];
    int fd_rw[children_amount][2];

    //Initializing select buffers
    fd_set read_fd_set, read_fd_set_aux;
    FD_ZERO(&read_fd_set);
    FD_ZERO(&read_fd_set_aux);

    int nfds = 0;  //Highest numbered file descriptor
    int index = 1; //Index of file to be sent to child

    //Creating children
    for(int i = 0 ; i < children_amount ; i++){

        creatingPipes(pipe_w_aux, pipe_r_aux, fd_rw[i]);

        //Giving child starting files
        index += sendChildFile(fd_rw[i][1], argc, argv, index, init_amount);
        FD_SET(fd_rw[i][0], &read_fd_set);  // Adding fds to rfds select argument

        createChild(pipe_w_aux, pipe_r_aux, i, children_amount, fd_rw, &nfds);
    }
    nfds++; //Select argument convention

    int retrieved = 0;

    while(retrieved < to_read){
        //Create aux set to execute select
        read_fd_set_aux = read_fd_set;
        int available = select(nfds, &read_fd_set_aux, NULL, NULL, NULL); // ASK (timeout/last argument):  select should 1) specify the timeout duration to wait for event 2) NULL: block indefinitely until one is ready 3) return immediately w/o blocking
        if(available == ERROR){
            errExit("Error in select function\n");
        }

        //Check what's available to read
        ssize_t aux;
        char aux_buff[READ_BUF_AUX_SIZE];
        for(int i = 0 ; i < children_amount && available != 0 ; i++) {
            if(FD_ISSET(fd_rw[i][0], &read_fd_set_aux)) {

                aux = readFromChild(fd_rw[i][0], aux_buff, shmp);

                aux_buff[aux] = '\0';

                if(initial_amount_read[i] != 0) {
                    initial_amount_read[i]--;
                }

                //If child finished with initial amount sent
                if(initial_amount_read[i] == 0) {
                    //Give an additional file to process
                    int cant_sent = sendChildFile(fd_rw[i][1], argc, argv, index, 1);
                    index += cant_sent;
                    //If there's not left files, close the pipes
                    if (cant_sent == 0) {
                        closePipe(fd_rw[i]);
                        FD_CLR(fd_rw[i][0], &read_fd_set);
                    }
                }

                writeOnShm(aux, shmp,aux_buff);

                //Informs that there is something to read
                if(sem_post(&(shmp->left_to_read)) == ERROR) {
                    errExit("Error in sem_post function\n");
                }
                //Writes in output file what child returned
                fprintf(file, "%s\n", aux_buff);

                retrieved++;
                available--;
            }
        }
    }

    finalClosings(file, shmp, shm_fd);
    return 0;
}

struct shmbuf * initializeShm(int *shm_fd){
    struct shmbuf * shmp;
    if(((*shm_fd) = shm_open(NAME_SHM, O_CREAT | O_RDWR | O_TRUNC, 0644)) == ERROR){
        errExit("Error in shm_open function while creating shared memory\n");
    }

    if(ftruncate((*shm_fd), sizeof(struct shmbuf)) == ERROR){
        errExit("Error in ftruncate function\n");
    }

    shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,MAP_SHARED, (*shm_fd), 0);
    if (shmp == MAP_FAILED){
        errExit("Error in mmap function\n");
    }

    // initializing semaphore in 0
    if (sem_init(&shmp->left_to_read, 1, 0) == ERROR){
        errExit("Error in sem_init function while initializing semaphore read\n");
    }
    return shmp;
}

void waitForView(){
    //Send shared memory name for view to receive
    printf("%s", NAME_SHM);
    sleep(2);
    printf("\n");
}

int manageInitialAmountOfFiles(int children_amount, int initial_amount_read[children_amount], int total_files){
    //Calculating significantly minor amount of initial files to send
    int init_amount = (int)(0.05 * total_files) + 1 ;

    if(FILES_PER_CHILD < init_amount){
        init_amount= FILES_PER_CHILD;
    }

    //Setting array of initial amount of files for later organization
    for(int i = 0; i < children_amount ; i++){
        initial_amount_read[i] = init_amount;
    }

    return init_amount;
}

void creatingPipes(int pipe_w_aux[2], int pipe_r_aux[2], int fd_rw[2]){
    if(pipe(pipe_w_aux) == ERROR){
        errExit("Error in pipe function while generating pipe_app\n");
    }
    fd_rw[1] = pipe_w_aux[1];

    if(pipe(pipe_r_aux) == ERROR){
        errExit("Error in pipe function while generating pipe_chld\n");
    }
    fd_rw[0] = pipe_r_aux[0];
}

int sendChildFile(int fd, int argc, char* argv[], int index, int cant_files){
    //Checking if there are more files to send
    if(cant_files == 0 || index == argc) {
        return 0;
    }
    //Sending file to child
    write(fd, argv[index], strlen(argv[index]));
    write(fd, "\n", 1); //File delimiter
    return 1 + sendChildFile(fd, argc, argv, index + 1, cant_files - 1);
}

void redirectPipes(int old_fd, int new_fd){
    dup2(old_fd, new_fd);
    close(old_fd);
}

void closeAuxPipes(int pipe_w_aux[2], int pipe_r_aux[2]){
    close(pipe_w_aux[0]);
    close(pipe_r_aux[1]);
}

void closePipe(int fd_rw[2]){
    close(fd_rw[0]);
    close(fd_rw[1]);
}

void closeParallelChildFD(int child, int fd_rw[child][2]){
    for (int i = 0 ; i <= child ; i++) {
        closePipe(fd_rw[i]);
    }
}

void createChild(int pipe_w_aux[2], int pipe_r_aux[2], int index, int children_amount, int fd_rw[children_amount][2], int *nfds){
    pid_t cpid = fork();

    if(cpid == ERROR){
        errExit("Error in fork function\n");
    }

    if(cpid == 0){  // child process
        char * new_argv[] = {CHILD, NULL};  // passing first files as argument
        char * new_envp[] = {NULL};

        redirectPipes(pipe_w_aux[0], STDIN_FILENO);  //child reading from stdin
        redirectPipes(pipe_r_aux[1], STDOUT_FILENO); //child writing to stdout

        closeParallelChildFD(index, fd_rw);

        execve(new_argv[0], new_argv, new_envp);
        //execve returns on error
        errExit("Error in execve\n");
    }
    closeAuxPipes(pipe_w_aux, pipe_r_aux);

    if(fd_rw[index][0] > *nfds){
        *nfds = fd_rw[index][0];
    }
}

ssize_t readFromChild(int fd, char aux_buff[READ_BUF_AUX_SIZE], struct shmbuf *shmp){
    ssize_t aux = read(fd, aux_buff, READ_BUF_AUX_SIZE);
    if(aux == ERROR) {
        errExit("Error in read function while reading from pipe\n");
    }
    if (aux == READ_BUF_AUX_SIZE) {
        errExit("Error in reading buffer. Enlarge aux buffer\n");
    }
    if(shmp->index_of_writing + aux > BUF_SIZE) {
        errExit("Error no space left on buffer\n");
    }
    return aux;
}

void writeOnShm(size_t aux, struct shmbuf *shmp, char * aux_buff){
    for (int i = 0 ; i < aux ; i++) {
        shmp->buf[shmp->index_of_writing + i] = aux_buff[i];
    }
    shmp->index_of_writing += aux;
    shmp->buf[shmp->index_of_writing++] = '\0';
}

void finalClosings(FILE * file, struct shmbuf * shmp, int shm_fd){
    fclose(file);
    munmap(shmp, sizeof(*shmp));
    close(shm_fd);
    shm_unlink(NAME_SHM);
}



