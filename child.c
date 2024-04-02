#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LEN 4096
#define SIZE 32

int main(int argc, char* argv[]) {

    //Reads file to use
    char files[MAX_LEN];
    ssize_t bytes_read;

    while ((bytes_read = read(STDIN_FILENO, files, sizeof(files))) > 0) {
        // NULL-terminated string
        files[bytes_read] = '\0';

        //Creating child to do md5
        pid_t md5_child;
        char *argv_chld[] = {"md5sum", files, NULL};
        char *envp_chld[] = {NULL};
        switch (md5_child = fork()) {
            case -1:        //fork failed
                printf("ERROR: fork() failed\n");
                return -1;
            case 0:
                char *file = strtok(files, " \t\n");
                //dup2(pipeFD[1], STDOUT_FILENO); //stdout in writing pipe
                //Calculating md5 of file
                while(file != NULL){
                    execve("/usr/bin/md5sum", argv_chld, envp_chld);
                    file = strtok(NULL, " \t\n");
                }
                return 0;
            default:
                wait(NULL);
                break;
        }
    }

    return 0;
}