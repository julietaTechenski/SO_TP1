#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LEN 1024
#define MD5_SIZE 32
#define MD5SUM "md5sum"
#define READING "r"

int main(int argc, char* argv[]) {
    pid_t pid = getpid();

    char files[MAX_LEN];
    ssize_t bytes_read;

    //Reading file to use
    while ((bytes_read = read(STDIN_FILENO, files, sizeof(files))) > 0) {
        if(files[bytes_read - 1] == '\n'){
            bytes_read--;
        }
        files[bytes_read] = '\0';

        //Building command
        int size = bytes_read + sizeof(MD5SUM) + 1;
        char command[size];
        sprintf(command, "%s %s", MD5SUM, files);
        command[size] = '\0';

        //Creating pipes for child_md5 process
        FILE * child_pipe = popen(command, READING);
        if (child_pipe == NULL) {
            printf("Error popen() failed");
            return -1;
        }

        //Reading output
        char md5[MD5_SIZE + 1];
        if (read(fileno(child_pipe), md5, MD5_SIZE) != MD5_SIZE) {
            printf("Error read() failed");
            return -1;
        }
        md5[MD5_SIZE] = '\0';

        pclose(child_pipe);

        //Returning md5 to parent
        printf("%s - %s - %d\n", files, md5, pid);
    }

    return 0;
}