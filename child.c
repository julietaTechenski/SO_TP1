#include "head.h"

#define MAX_PID 5
#define MD5_SIZE 32
#define MD5SUM "md5sum"
#define READING "r"
#define MAX_FORMAT 10 //extra characters for format (spaces, -, :, etc)

void creatingMD5Child(char * file, int file_size, char * md5){
    //Building command
    int aux_size = file_size + sizeof(MD5SUM) + 2;  //space and '\0'
    char command[aux_size];
    int length = snprintf(command, aux_size,"%s %s", MD5SUM, file);

    if (length < 0 || length >= aux_size)
        errExit("snprintf creating md5_child");

    //Creating pipes for child_md5 process
    FILE * child_pipe = popen(command, READING);
    if (child_pipe == NULL)
        errExit("Error popen() failed");

    //Reading output
    if (read(fileno(child_pipe), md5, MD5_SIZE) != MD5_SIZE)
        errExit("Error read() failed");
    md5[MD5_SIZE] = '\0';

    pclose(child_pipe);
}

void generateAnswer(char * file, int size_file, char * md5, int pid){
    int aux_size = size_file + MD5_SIZE + MAX_PID + MAX_FORMAT;
    char answer[aux_size];
    int length = snprintf(answer, aux_size, "%s - %s - %d\n", file, md5, pid);

    if (length < 0 || length >= aux_size)
        errExit("snprintf generating answer");

    if (write(STDOUT_FILENO, answer, length) != length)
        errExit("Error writing to parent pipe from child");
}

int main(int argc, char* argv[]) {

    pid_t pid = getpid();

    char * file = NULL;
    size_t size_of_file_buff = 0;
    size_t aux_char_read;

    char md5[MD5_SIZE + 1];

    //Reading file to use
    while ((aux_char_read = getline(&file, &size_of_file_buff, stdin)) != -1) {
        aux_char_read--; //deleting \n
        file[aux_char_read] = '\0';
        creatingMD5Child(file, aux_char_read, md5);
        generateAnswer(file, aux_char_read, md5, pid);
    }

    free(file);
    return 0;
}