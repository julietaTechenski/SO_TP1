#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// struct: file name - md5 - ID child

#define SIZE 32

int main(int argc, char* argv[]){

    //Calculating md5 of file (argv[1])
    char command[100];
    snprintf(command, sizeof(command), "md5sum %s", argv[1]);

    system(command);

    /*write to through pipe to parent process:
     *  file_name (argv[1] (?)) - md5 - ID child (getpid())
     * (child processes do not write directly to shared memory)
     * */

    return 0;
}