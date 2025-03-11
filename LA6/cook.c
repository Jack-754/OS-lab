#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>  
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define F 1100
#define B 1101
#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

struct sembuf pop, vop;
int semmutex, semcook, semwaiter[5], semcustomers, shmid;
int *M;

void printtime(int t){
    int hr=t/60+11;
    int min=t%60;
    char ampm[3]="am";
    if(hr>12){
        hr-=12;
        strcpy(ampm, "pm");
    }
    if(min>9)printf("[%2d:%d %s] ", hr, min, ampm);
    else printf("[%2d:0%d %s] ", hr, min, ampm);
    fflush(stdout);
}

void signalhandler(int s){
    shmdt(M);
    exit(0);
}

int max(int a, int b){
    if(a>b){
        printf("Time setting failed.\n");
        exit(0);
    }
    return b;
}

void cmain(char cook){
    P(semmutex);
    printtime(M[0]);
    printf("Cook %c is ready\n", cook);
    fflush(stdout);
    V(semmutex);
    while(1){
        P(semcook);
        P(semmutex);
        if(M[0]>240 && M[3]==0){
            printtime(M[0]);
            printf("Cook %c: Leaving\n", cook);
            for(int i=0; i<5; i++){
                V(semwaiter[i]);
            }
            V(semmutex);
            break;
        }
        int waiter, customerIdx, count;
        waiter=M[M[F]++];
        customerIdx=M[M[F]++];
        count=M[M[F]++];
        int cur_time=M[0];
        printtime(M[0]);
        printf("Cook %c: Preparing order (Waiter %c, Customer %d, Count %d)\n", cook, 'U'+waiter, customerIdx+1, count);
        fflush(stdout);
        V(semmutex);
        usleep(count*5*(100000));
        P(semmutex);
        M[3]--;
        M[0]=max(M[0], cur_time+count*5);
        printtime(M[0]);
        printf("Cook %c: Prepared order (Waiter %c, Customer %d, Count %d)\n", cook, 'U'+waiter, customerIdx+1, count);
        fflush(stdout);
        int fr, po, f, b;
        fr=waiter*200+100;
        po=fr+1;
        f=po+1;
        b=f+1;
        M[fr]=customerIdx;
        if(M[0]>240 && M[4]==2 && M[3]==0){
            M[4]--;
            printtime(M[0]);
            printf("Cook %c: Leaving\n", cook);
            V(semwaiter[waiter]);
            V(semcook);
            V(semmutex);
            break;
        }
        else if(M[0]>240 && M[4]==1){
            printtime(M[0]);
            printf("Cook %c: Leaving\n", cook);
            V(semwaiter[waiter]);
            for(int i=0; i<5; i++){
                V(semwaiter[i]);
            }
            V(semmutex);
            break;
        }
        V(semwaiter[waiter]);
        V(semmutex);
    }
    shmdt(M);
    exit(0);
}


int main(){
    signal(SIGINT, signalhandler);

    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1;

    int keysemmutex, keysemcook, keysemwaiter[5], keysemcustomers, keyshmid;
    keysemmutex=ftok("makefile", 1);
    keysemcook=ftok("makefile", 2);
    keysemwaiter[0]=ftok("makefile", 3);
    keysemwaiter[1]=ftok("makefile", 4);
    keysemwaiter[2]=ftok("makefile", 5);
    keysemwaiter[3]=ftok("makefile", 6);
    keysemwaiter[4]=ftok("makefile", 7);
    keysemcustomers=ftok("makefile", 8);
    keyshmid=ftok("makefile", 9);

    semmutex = semget(keysemmutex, 1, 0777|IPC_CREAT);
	semcook = semget(keysemcook, 1, 0777|IPC_CREAT);
    semwaiter[0] = semget(keysemwaiter[0], 1, 0777|IPC_CREAT);
    semwaiter[1] = semget(keysemwaiter[1], 1, 0777|IPC_CREAT);
    semwaiter[2] = semget(keysemwaiter[2], 1, 0777|IPC_CREAT);
    semwaiter[3] = semget(keysemwaiter[3], 1, 0777|IPC_CREAT);
    semwaiter[4] = semget(keysemwaiter[4], 1, 0777|IPC_CREAT);
    semcustomers = semget(keysemcustomers, 200, 0777|IPC_CREAT);
    shmid = shmget(keyshmid, 2000*sizeof(int), 0777|IPC_CREAT);

    semctl(semmutex, 0, SETVAL, 1);
    semctl(semcook, 0, SETVAL, 0);
    semctl(semwaiter[0], 0, SETVAL, 0);
    semctl(semwaiter[1], 0, SETVAL, 0);
    semctl(semwaiter[2], 0, SETVAL, 0);
    semctl(semwaiter[3], 0, SETVAL, 0);
    semctl(semwaiter[4], 0, SETVAL, 0);
    for(int i=0; i<200; i++){
        semctl(semcustomers, i, SETVAL, 0);
    }

    M=shmat(shmid, 0, 0);
    for(int i=0; i<2000; i++)M[i]=0;

    P(semmutex);
    M[1]=10;
    M[4]=2;         // number of cooks currently
    M[100]=-1;
    M[102]=104;
    M[103]=104;
    M[300]=-1;
    M[302]=304;
    M[303]=304;
    M[500]=-1;
    M[502]=504;
    M[503]=504;
    M[700]=-1;
    M[702]=704;
    M[703]=704;
    M[900]=-1;
    M[902]=904;
    M[903]=904;
    M[1100]=1102;
    M[1101]=1102;
    V(semmutex);


    char cook = 'C';
    for(int i=0; i<2; i++){
        if(!fork()){
            cmain(cook);
        }
        cook++;
    }

    for(int i=0; i<2; i++){
        wait(NULL);
    }

    shmdt(M);
    return 0;


}