#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[]){
    int children_amount = (argc-1)/10;


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

}