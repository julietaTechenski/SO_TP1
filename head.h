#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>  // for shm
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>

#define CHILD "./child"
#define NAME_SHM "/app_shm"
#define FILE_NAME "md5_output.txt"

#define READING "r"
#define WRITING "w"

#define BUF_SIZE 4096
#define ERROR -1

#define errExit(msg) {perror(msg); exit(EXIT_FAILURE);}

struct shmbuf{
    int cant_files_to_print;
    sem_t  left_to_read;                /* POSIX unnamed semaphore */
    size_t index_of_reading;            /* Index of reading */
    size_t index_of_writing;            /* Number of bytes used in 'buf' */
    char buf[BUF_SIZE];                 /* Data being transferred */
};

struct shmbuf * initializeShm(int *shm_fd);
void waitForView();
int manageInitialAmountOfFiles(int children_amount, int initial_amount_read[children_amount], int total_files);
void redirectPipes(int old_fd, int new_fd);
void creatingPipes(int pipe_w_aux[2], int pipe_r_aux[2], int fd_rw[2]);
void closeAuxPipes(int pipe_w_aux[2], int pipe_r_aux[2]);
void closePipe(int fd_rw[2]);
void closeParallelChildFD(int child, int fd_rw[child][2]);
void createChild(int pipe_w_aux[2], int pipe_r_aux[2], int index, int children_amount, int fd_rw[children_amount][2], int *nfds);
int sendChildFile(int fd, int argc, char* argv[], int index, int cant_files);
void finalClosings(FILE * file, struct shmbuf * shmp, int shm_fd);



