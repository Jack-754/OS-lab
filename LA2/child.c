#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>


#define PLAY    0
#define CATCH   1
#define MISS    2
#define OUT     3

int n;
int *proc;
int cur=PLAY;
int idx;
int pid;

void handler( int sig ){
    if (sig == SIGUSR1) {
        if(cur==PLAY){
            printf("....    ");
        }
        else if(cur==CATCH){
            printf("CATCH   ");
            cur=PLAY;
        }
        else if(cur==MISS){
            printf("MISS    ");
            cur=OUT;
        }
        else if(cur==OUT){
            printf("        ");
        }
        fflush(stdout);
        if(idx==n-1){
            FILE *file=fopen("dummycpid.txt", "r");
            int D;
            fscanf(file, "%d", &D);
            fclose(file);
            kill(D, SIGINT);
        }
        else{
            kill(proc[idx+1], SIGUSR1);
        }
    } 
    else if (sig == SIGUSR2) {
        int random=rand();
        FILE* file=fopen("test.txt", "a");
        fprintf(file, "%d %d %d\n", idx, pid, random);
        fclose(file);
        random%=5;
        int ppid=getppid();
        if(random==0){
            cur=MISS;
            kill(ppid, SIGUSR2);
        }
        else{
            cur=CATCH;
            kill(ppid, SIGUSR1);
        }
    }
    else if(sig == SIGINT){
        if(cur==PLAY){
            printf("+++ Child %d: Yay! I am the winner!\n", idx+1);
            fflush(stdout);
        }
        free(proc);
        exit(0);
    }
}

int main(){
    pid=getpid();
    srand(pid);
    sleep(1);
    FILE *file=fopen("childpid.txt", "r");
    fscanf(file, "%d", &n);
    proc=malloc(sizeof(int)*n);
    for(int i=0; i<n; i++){
        fscanf(file, "%d", &proc[i]);
        if(proc[i]==pid)idx=i;
    }
    fclose(file);

    signal(SIGUSR1, handler);           
    signal(SIGUSR2, handler);
    signal(SIGINT, handler);

    while(1)pause();
    

    return 0;
}