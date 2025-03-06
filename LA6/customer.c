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

int max(int a, int b){
    if(a>b){
        printf("Time setting failed.\n");
        exit(0);
    }
    return b;
}

void signalhandler(int s){
    semctl(semmutex, 0, IPC_RMID, 0);
	semctl(semcook, 0, IPC_RMID, 0);
    semctl(semwaiter[0], 0, IPC_RMID, 0);
    semctl(semwaiter[1], 0, IPC_RMID, 0);
    semctl(semwaiter[2], 0, IPC_RMID, 0);
    semctl(semwaiter[3], 0, IPC_RMID, 0);
    semctl(semwaiter[4], 0, IPC_RMID, 0);
    semctl(semcustomers, 0, IPC_RMID, 0);
    shmdt(M);
    shmctl(shmid, IPC_RMID, 0);
    exit(0);
}

void cmain(int idx, int arr, int count){
    P(semmutex);
    M[0]=max(M[0], arr);
    if(M[0]>240){
        printtime(M[0]);
        printf("Customer %d leaves (arrival after closing)\n", idx+1);
        V(semmutex);
        exit(0);
    }
    if(M[1]==0){
        printtime(M[0]);
        printf("Customer %d leaves (no empty table)\n", idx+1);
        V(semmutex);
        exit(0);
    }
    M[1]--;
    M[3]++;
    printtime(M[0]);
    printf("Customer %d arrives (count = %d)\n", idx+1, count);
    int fr, po, f, b, waiter;
    waiter=M[2];
    fr=M[2]*200+100;
    po=fr+1;
    f=po+1;
    b=f+1;
    M[2]=(M[2]+1)%5;
    M[po]++;
    M[M[b]++]=idx;
    M[M[b]++]=count;
    V(semwaiter[waiter]);
    V(semmutex);
    Pi(semcustomers, idx);

    P(semmutex);
    printtime(M[0]);
    printf("Customer %d: Order placed to Waiter %c\n", idx+1, 'U'+waiter);
    V(semmutex);

    Pi(semcustomers, idx);
    P(semmutex);
    printtime(M[0]);
    printf("Customer %d gets food [Waiting time = %d]\n", idx+1, M[0]-arr);
    fflush(stdout);
    int cur_time=M[0];
    V(semmutex);
    usleep(30*100000);
    P(semmutex);
    M[0]=max(M[0], cur_time+30);
    printtime(M[0]);
    printf("Customer %d finishes eating and leaves\n", idx+1);
    fflush(stdout);
    M[1]++;
    V(semmutex);
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

	FILE *file=fopen("customers.txt", "r");
    if(file==NULL){
        printf("File not found\n");
        return 0;
    }
    int idx, arr, count, n=0, last=0;
    while(1){
        fscanf(file, "%d", &idx);
        if(idx==-1)break;
        n++;
        fscanf(file, "%d %d", &arr, &count);
        usleep((arr-last)*100000);
        last=arr;
        if(!fork()){
            cmain(idx-1, arr, count);
        }
    }

    fclose(file);
    while(n--){
        wait(NULL);
    }

    semctl(semmutex, 0, IPC_RMID, 0);
	semctl(semcook, 0, IPC_RMID, 0);
    semctl(semwaiter[0], 0, IPC_RMID, 0);
    semctl(semwaiter[1], 0, IPC_RMID, 0);
    semctl(semwaiter[2], 0, IPC_RMID, 0);
    semctl(semwaiter[3], 0, IPC_RMID, 0);
    semctl(semwaiter[4], 0, IPC_RMID, 0);
    semctl(semcustomers, 0, IPC_RMID, 0);
    shmdt(M);
    shmctl(shmid, IPC_RMID, 0);

    return 0;
}