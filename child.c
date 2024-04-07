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


        //Creating child to do md5
        pid_t md5_child;
        char *argv_chld[] = {"md5sum", files, NULL};
        char *envp_chld[] = {NULL};
        switch (md5_child = fork()) {
            case -1:        //fork failed
                printf("ERROR: fork() failed\n");
                return -1;
            case 0:

                char *file = strtok(files, " \t\n");    //generate token
                //Calculating md5 of file
                while(file != NULL){
                    execve("/usr/bin/md5sum", argv_chld, envp_chld);
                    file = strtok(NULL, " \t\n");
                }
                return 0;
            default:

                //Returning md5 to parent
                int size = MD5_SIZE;    //md5
                size += bytes_read;     //file name
                size++;                 //space
                char md5_fileName[size];
                md5_fileName[size] = '\0';

                //Sending md5 of file
                printf("%s %d\n", md5_fileName, pid);
                wait(NULL);

                break;
        }
    }

    return 0;
}