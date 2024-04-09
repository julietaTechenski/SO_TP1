#include "head.h"

#define MAX_LEN 1024
#define MD5_SIZE 32
#define MD5SUM "md5sum"
#define READING "r"

void creatingMD5Child(char * file, int file_size, char * md5){
    //Building command
    int size = file_size + sizeof(MD5SUM) + 1;
    char command[size];
    sprintf(command, "%s %s", MD5SUM, file);
    command[size] = '\0';

    //Creating pipes for child_md5 process
    FILE * child_pipe = popen(command, READING);
    if (child_pipe == NULL) {
        errExit("Error popen() failed");
    }

    //Reading output
    if (read(fileno(child_pipe), md5, MD5_SIZE) != MD5_SIZE) {
        errExit("Error read() failed");
    }
    md5[MD5_SIZE] = '\0';

    pclose(child_pipe);
    return;
}

void generateAnswer(char * file, char * md5, int pid){
    char answer[MAX_LEN + MD5_SIZE + 50];
    int length = snprintf(answer, sizeof(answer), "%s - %s - %d\n", file, md5, pid);

    if (length < 0 || length >= sizeof(answer)) {
        errExit("snprintf");
    }

    if (write(STDOUT_FILENO, answer, length) != length) {
        errExit("Error writing to parent pipe from child");
    }
    return;
}

int main(int argc, char* argv[]) {
    pid_t pid = getpid();

    char files[MAX_LEN];
    ssize_t bytes_read;

    //Reading file to use
    while ((bytes_read = read(STDIN_FILENO, files, MAX_LEN)) > 0) {
        files[bytes_read] = 0;

        char md5[MD5_SIZE + 1];
        creatingMD5Child(files, bytes_read, md5);

        generateAnswer(files, md5, pid);

    }

    return 0;
}