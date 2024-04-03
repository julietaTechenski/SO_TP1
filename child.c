#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LEN 1024
#define MD5_SIZE 32

int main(int argc, char* argv[]) {
    pid_t pid = getpid();

    char files[MAX_LEN];
    ssize_t bytes_read;

    //Reading file to use
    while ((bytes_read = read(STDIN_FILENO, files, sizeof(files))) > 0) {

        //Creating pipes for child_md5 process
        int pipeFD[2];
        if(pipe(pipeFD) == -1){
            printf("ERROR: pipe() failed\n");
            return -1;
        }

        //Saving STDOUT_FILENO before redirecting
        int stdout = dup(STDOUT_FILENO);

        //Creating child to do md5
        pid_t md5_child;
        char *argv_chld[] = {"md5sum", files, NULL};
        char *envp_chld[] = {NULL};
        switch (md5_child = fork()) {
            case -1:        //fork failed
                printf("ERROR: fork() failed\n");
                return -1;
            case 0:
                //Managing children piping
                close(pipeFD[0]); // unused r-end
                dup2(pipeFD[1], STDOUT_FILENO); //stdout in writing pipe
                close(pipeFD[1]); // duplicate w-end

                char *file = strtok(files, " \t\n");    //generate token
                //Calculating md5 of file
                while(file != NULL){
                    execve("/usr/bin/md5sum", argv_chld, envp_chld);
                    file = strtok(NULL, " \t\n");
                }
                return 0;
            default:
                dup2(stdout, STDOUT_FILENO);    //recovers stdout

                //Returning md5 to parent
                int size = MD5_SIZE;    //md5
                size += bytes_read;     //file name
                size++;                 //space
                char md5_fileName[size];
                read(pipeFD[0], md5_fileName, size);
                md5_fileName[size] = '\0';

                //Sending md5 of file
                printf("%s %d\n", md5_fileName, pid);
                wait(NULL);

                break;
        }
        //Closing pipes
        close(stdout);
        close(pipeFD[0]);
        close(pipeFD[1]);
    }

    return 0;
}