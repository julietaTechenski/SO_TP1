#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define SIZE 32

int main(int argc, char* argv[]){

    int pipeFD[2];

    if(pipe(pipeFD) == -1){
        printf("ERROR: pipe() failed\n");
        return -1;
    }

    //Saving STDOUT_FILENO before redirecting
    int stdout = dup(STDOUT_FILENO);

    dup2(pipeFD[1], STDOUT_FILENO); //stdout in writing pipe

    //Calculating md5 of file (argv[1])
    char command[100];
    snprintf(command, sizeof(command), "md5sum %s", argv[1]);

    system(command);

    //Returning md5 to parent
    char md5[SIZE];
    read(pipeFD[0], md5, SIZE);

    //Recovering stdout to send back to parent file
    dup2(stdout, STDOUT_FILENO);

    //Sending md5 of file
    write(STDOUT_FILENO, md5, SIZE);
    write(STDOUT_FILENO, "\n", 1);

    close(stdout);
    close(pipeFD[0]);
    close(pipeFD[1]);

    return 0;
}