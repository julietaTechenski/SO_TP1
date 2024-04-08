#include "head.h"

#define MAX_LEN 1024
#define MD5_SIZE 32
#define MD5SUM "md5sum"
#define READING "r"

int main(int argc, char* argv[]) {
    pid_t pid = getpid();

    char files[MAX_LEN];
    ssize_t bytes_read;

    //Reading file to use
    while ((bytes_read = read(STDIN_FILENO, files, MAX_LEN)) > 0) {
        files[bytes_read] = 0;

        //Building command
        int size = bytes_read + sizeof(MD5SUM) + 1;
        char command[size];
        sprintf(command, "%s %s", MD5SUM, files);
        command[size] = '\0';

        //Creating pipes for child_md5 process
        FILE * child_pipe = popen(command, READING);
        if (child_pipe == NULL) {
            errExit("Error popen() failed");
        }

        //Reading output
        char md5[MD5_SIZE + 1];
        if (read(fileno(child_pipe), md5, MD5_SIZE) != MD5_SIZE) {
            errExit("Error read() failed");
        }
        md5[MD5_SIZE] = '\0';

        pclose(child_pipe);

        char buffer[MAX_LEN + MD5_SIZE + 50];  // Ajusta el tamaño según necesites
        int length = snprintf(buffer, sizeof(buffer), "%s - %s - %d\n", files, md5, pid);
        if (length < 0 || length >= sizeof(buffer)) {
            errExit("snprintf");
        }

        if (write(STDOUT_FILENO, buffer, length) != length) {
            errExit("Error writing to parent pipe from child");
        }
    }

    return 0;
}