#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>  // for shm
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>

#define CHILD "./child"
#define BUF_SIZE 4096
#define NAME_SHM "/app_shm"
#define MAX_PATH_LENGTH 128

#define errExit(msg) {perror(msg); exit(EXIT_FAILURE);}

struct shmbuf{
    sem_t  sem_read;            /* POSIX unnamed semaphore */
    size_t index;             /* Index of reading */
    size_t cnt;             /* Number of bytes used in 'buf' */
    char buf[BUF_SIZE];   /* Data being transferred */
};