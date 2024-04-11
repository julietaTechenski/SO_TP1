#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>  // for shm
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>

#define CHILD "./child"
#define NAME_SHM "/app_shm"

#define errExit(msg) {perror(msg); exit(EXIT_FAILURE);}

#define BUF_SIZE 4096
struct shmbuf{
    int cant_files_to_print;
    sem_t  left_to_read;            /* POSIX unnamed semaphore */
    size_t index_of_reading;             /* Index of reading */
    size_t index_of_writing;             /* Number of bytes used in 'buf' */
    char buf[BUF_SIZE];   /* Data being transferred */
};
