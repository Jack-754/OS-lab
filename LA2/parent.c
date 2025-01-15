#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int next=0;
int cnt;
int *active;
int n;

void childSigHandler ( int sig ){
    if (sig == SIGUSR1) {
        next++;
    } 
    else if (sig == SIGUSR2) {
        cnt--;
        active[next]=0;
        next++;
    }
    if(next>=n)next=0;
}

int main(int argc, char *argv[]){
    if(argc<2){
        printf("Enter value of n!");
        fflush(stdout);
        exit(0);
    }
    n=atoi(argv[1]);
    int pid;
    int proc[n];
    cnt=n;
    active=malloc(sizeof(int)*n);

    FILE* file=fopen("childpid.txt", "w");
    fprintf(file, "%d\n", n);
    for(int i=0; i<n; i++){
        pid=fork();
        if(pid==0){
            execlp("./child", "./child", NULL);
        }
        else{
            proc[i]=pid;
            fprintf(file, "%d\n", pid);
            active[i]=1;
        }
    }
    fclose(file);
    printf("Parent: %d child processes created.\n", n);
    printf("Waiting for child processes to read database.\n");
    sleep(2);

    printf("\t");
    for(int i=0; i<n; i++){
        printf("%d\t", i);
    }
    printf("\n");
    fflush(stdout);

    signal(SIGUSR1, childSigHandler);    
    signal(SIGUSR2, childSigHandler);
    next=0;
    while(cnt>1){
        printf("+----");
        for(int i=0; i<n*8; i++){
            printf("-");
        }
        printf("----+\n");
        fflush(stdout);
        for(int i=0; i<n; i++){
            if(active[next]==0)next++;
            else break;
            if(next>=n)next=0;
        }
        kill(proc[next], SIGUSR2);
        pause();
        printf("|    ");
        fflush(stdout);

        pid=fork();
        if(pid==0){
            execlp("./dummy", "./dummy", NULL);
        }
        FILE* file=fopen("dummycpid.txt", "w");
        fprintf(file, "%d", pid);
        fclose(file);
        kill(proc[0], SIGUSR1);
        int status;
        waitpid(pid, &status, 0);
        printf("    |\n");
        fflush(stdout);
    }

    printf("+----");
    for(int i=0; i<n*8; i++){
        printf("-");
    }
    printf("----+\n");

    printf("\t");
    for(int i=0; i<n; i++){
        printf("%d\t", i);
    }
    printf("\n");

    fflush(stdout);
    for(int i=0; i<n; i++){
        kill(proc[i], SIGINT);
    }
    free(active);


    return 0;
}