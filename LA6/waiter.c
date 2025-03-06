#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>  
#include <signal.h>
#include <stdlib.h>
#include <string.h>

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

void Pi(int s, int i){
    pop.sem_num = i;
    semop(s, &pop, 1);
    pop.sem_num = 0;
}

void Vi(int s, int i){
    vop.sem_num = i;
    semop(s, &vop, 1);
    vop.sem_num = 0;
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

void wmain(int idx){
    printtime(M[0]);
    printf("Waiter %c is ready\n", 'U'+idx);
    fflush(stdout);
    int fr, po, f, b;
    fr=idx*200+100;
    po=fr+1;
    f=po+1;
    b=f+1;
    while(1){
        P(semwaiter[idx]);
        P(semmutex);
        if(M[fr]>=0){
            printtime(M[0]);
            printf("Waiter %c: Serving food to Customer %d\n", 'U'+idx, M[fr]+1);
            fflush(stdout);
            Vi(semcustomers, M[fr]);
            M[fr]=-1;
        }
        else if(M[po]>0){
            int customerIdx, count;
            customerIdx=M[M[f]++];
            count=M[M[f]++];
            M[po]--;
            int cur_time=M[0];
            V(semmutex);
            usleep(100000);
            P(semmutex);
            M[0]=max(M[0], cur_time+1);
            M[M[B]++]=idx;
            M[M[B]++]=customerIdx;
            M[M[B]++]=count;
            printtime(M[0]);
            printf("Waiter %c: Placing order for Customer %d (count = %d)\n", 'U'+idx, customerIdx+1, count);
            fflush(stdout);
            Vi(semcustomers, customerIdx);
            V(semcook);
        }
        else if(M[0]>240){
            printtime(M[0]);
            printf("Waiter %c: Leaving\n", 'U'+idx);
            fflush(stdout);
            V(semmutex);
            break;
        }   
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

    semmutex = semget(keysemmutex, 1, 0777);
	semcook = semget(keysemcook, 1, 0777);
    semwaiter[0] = semget(keysemwaiter[0], 1, 0777);
    semwaiter[1] = semget(keysemwaiter[1], 1, 0777);
    semwaiter[2] = semget(keysemwaiter[2], 1, 0777);
    semwaiter[3] = semget(keysemwaiter[3], 1, 0777);
    semwaiter[4] = semget(keysemwaiter[4], 1, 0777);
    semcustomers = semget(keysemcustomers, 200, 0777);
    shmid = shmget(keyshmid, 2000*sizeof(int), 0777);

    M=shmat(shmid, 0, 0);

    for(int i=0; i<5; i++){
        if(!fork()){
            wmain(i);
        }
    }

    for(int i=0; i<5; i++){
        wait(NULL);
    }

    shmdt(M);
    return 0;
}