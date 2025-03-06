#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>  
#include <signal.h>
#include <stdlib.h>

int main(){
    printf("done\n");
    usleep(10000000);
    printf("done\n");
    return 0;
}