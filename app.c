#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>  // for shm
#include <sys/select.h>

int main(int argc, char* argv[]){
    int children_amount = (argc-1)/10;

    char *shmpath = "shm.txt";
    int shmfd;

    if((shmfd = shm_open(shmpath, O_CREAT | O_RDWR | O_TRUNC, 0644)) == -1){
        perror("Error creating shared memory file\n");
        exit(EXIT_FAILURE);
    }

//    shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,
//                MAP_SHARED, fd, 0);
//    if (shmp == MAP_FAILED)
//        errExit("mmap");

/*int shm_open(const char *name, int oflag, mode_t mode);*/

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
            exit(EXIT_SUCESS);
        }
        if(nfds < cpid[i] || i == 0)
            nfds = cpid[i];
    }

    nfds++; // select argument convention

    while(to_read > 0){
        int available = select(nfds, &rfds, NULL, NULL, NULL); // ASK (timeout/last argument):  select should 1) specify the timeout duration to wait for event 2) NULL: block indefinitely until one is ready 3) return immediately w/o blocking
        while(available > 0) {
            /*reads*/
            available--;
        }
        to_read--;
    }
    return 0;
}
