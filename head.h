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

/**
 * @def this function opens the shared memory, checks if its empty if it already existed
 * and sets semaphore left_to_read to zero.
 * @param shared memory file descriptor variable
 * @return shared memory struct
 */
struct shmbuf * initializeShm(int *shm_fd);

/**
 * @def this function waits 2 seconds for view.c to start
 */
void waitForView();

/**
 * @def calculates the initial amount of files to send to children and sets array for later reading and
 * future assigment purposes
 * @param children_amount
 * @param initial_amount_read array to check if child finished processing initial amount of files
 * @param total_files
 * @return
 */
int manageInitialAmountOfFiles(int children_amount, int initial_amount_read[children_amount], int total_files);

/**
 * @def creating reading and writing pipe
 * @param pipe_w_aux
 * @param pipe_r_aux
 * @param fd_rw
 */
void creatingPipes(int pipe_w_aux[2], int pipe_r_aux[2], int fd_rw[2]);

/**
 * @def tries to send cant_files to child if there are any
 * @param fd file descriptor of child
 * @param argc amount of files to read + 1
 * @param argv array of files
 * @param index current file to read
 * @param cant_files amount of files to send to the child
 * @return amount of files sent to child
 */
int sendChildFile(int fd, int argc, char* argv[], int index, int cant_files);

/**
 * @def redirecting children pipes to stdin and stdout
 * @param old_fd
 * @param new_fd
 */
void redirectPipes(int old_fd, int new_fd);

/**
 * @def closes reading and writing pipe
 * @param fd_rw
 */
void closePipe(int fd_rw[2]);

/**
 * @def closes previously created children pipes and unused of the child
 * @param child
 * @param fd_rw
 */
void closeParallelChildFD(int child, int fd_rw[child][2]);

/**
 * @def closes unused app pipes
 * @param pipe_w_aux
 * @param pipe_r_aux
 */
void closeAuxPipes(int pipe_w_aux[2], int pipe_r_aux[2]);

/**
 * @def creates child and sets pipes for future usage
 * @param pipe_w_aux
 * @param pipe_r_aux
 * @param index
 * @param children_amount
 * @param fd_rw
 * @param nfds
 */
void createChild(int pipe_w_aux[2], int pipe_r_aux[2], int index, int children_amount, int fd_rw[children_amount][2], int *nfds);

/**
 * @def manages if there was an error in function read or no more space in buffer
 * @param aux
 * @param shm
 */
void checkReadingErrors(size_t aux, struct shmbuf *shm);

/**
 * @def writes in shared memory what child returned
 * @param aux
 * @param shmp
 * @param aux_buff
 */
void writeOnShm(size_t aux, struct shmbuf *shmp, char * aux_buff);

/**
 * @def closes app resources
 * @param file
 * @param shmp
 * @param shm_fd
 */
void finalClosings(FILE * file, struct shmbuf * shmp, int shm_fd);
