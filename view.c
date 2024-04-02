#include <stdio.h>
#include <signal.h>

volatile sig_atomic_t shutdown_flag = 0;

void shm_unlink_handler(int sig) {
    if(sig == SIGINT){
        shutdown_flag = 1;
    }
}

/*
 * stdin as parameter: pathname of a shared memory object and the string that is to be copied
 * */
int main(int argc, char* argv[]){

    signal(SIGINT, shm_unlink_handler);

    while(!shutdown_flag){
        /*printing from shared memory*/
    }

    return 0;
}