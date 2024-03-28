#include <stdio.h>
#include <string.h>
// struct: file name - md5 - ID child

int main(int argc, char* argv[]){

    char *command = strcat("mdsum ", argv[1]);

    unsigned int md5 = system(command);

    /*write to shared memory:
     *    file_name (argv[1])    md5    getpid()
     * */

    return 0;
}