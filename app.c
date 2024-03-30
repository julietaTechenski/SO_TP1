#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>  // for shm
#include <sys/select.h>

int main(int argc, char* argv[]){
    int children_amount = (argc-1)/10;

    char *shmpath;
//  struct shmbuf  *shmp;

    if((shmpath = open("shm.txt", O_CREAT | O_RDWR | O_TRUNC, 0644)) == -1){
        perror("Error creating shared memory file\n");
        exit(EXIT_FAILURE);
    }

//    shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,
//                MAP_SHARED, fd, 0);
//    if (shmp == MAP_FAILED)
//        errExit("mmap");

    int shm_fd = shm(shmpath);
    if(shm_fd < 0){
        perror("Error establishing shared memory access\n");
        exit(EXIT_FAILURE);
    }
/*int shm_open(const char *name, int oflag, mode_t mode);*/


    fd_set rfds;
    FD_ZERO(&rfds);

    for(int i = 0; i < children_amount; i++){
        int pid = fork();
        if(pid < 0){
            perror("Error creating child process\n");
            exit(EXIT_FAILURE);
        }
        if(pid == 0){  // child process
            char *newargv[] = {"child", argv[i]};
            char *newenviron[] = {NULL};

            execve(newargv[0], newargv,newenviron);
        }
    }


    return 0;
}