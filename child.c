#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define SIZE 32

int main(int argc, char* argv[]){

    //Calculating md5 of file (argv[1])
    char command[100];
    snprintf(command, sizeof(command), "md5sum %s", argv[1]);

    system(command);

    return 0;
}