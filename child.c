#include <stdio.h>
#include <string.h>
// struct: file name - md5 - ID child

int main(int argc, char* argv[]){

    char *command = strcat("mdsum ", argv[1]);

    unsigned int md5 = system(command);

    /*write to through pipe to parent process:
     *  file_name (argv[1] (?)) - md5 - ID child (getpid())
     * (child processes do not write directly to shared memory)
     * */

    return 0;
}