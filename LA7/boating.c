#include <stdio.h>
#include <stdlib.h>
#include <string.h>   
#include <time.h>   
#include <unistd.h>  
#include <pthread.h>  


typedef struct semaphore{
    int value;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
}semaphore;

void P(semaphore *s){
    pthread_mutex_lock(&s->mtx);
    s->value--;
    while(s->value<0){
        pthread_cond_wait(&s->cv, &s->mtx);
    }
    pthread_mutex_unlock(&s->mtx);
}

void V(semaphore *s){
    pthread_mutex_lock(&s->mtx);
    s->value++;
    if(s->value<=0){
        pthread_cond_signal(&s->cv);
    }
    pthread_mutex_unlock(&s->mtx);
}

semaphore boat={0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
semaphore rider={0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
pthread_mutex_t bmtx=PTHREAD_MUTEX_INITIALIZER;

int m, n;
int BA[10000], BC[10000], BT[10000], remainingvistors, remainingboats;
pthread_barrier_t barriers[10000], EOS;

void minsleep(int min){
    usleep(min*100000);
}

void *trider(void *targ){
    int id = (int)targ;
    int vtime=30+rand()%91; // for vtime from 30 to 120 minutes (both inclusive)
    int rtime=15+rand()%46; // for rtime from 15 to 60 minutes (both inclusive)
    printf("Visitor\t%4d\tStarts sightseeing for %3d minutes\n", id+1, vtime);
    minsleep(vtime);
    printf("Visitor\t%4d\tReady to ride a boat (ride time = %d)\n", id+1, rtime);
    V(&boat);
    P(&rider);
    int free=-1;
    while(1){   
        pthread_mutex_lock(&bmtx);
        for(int i=0; i<m; i++){
            if(BA[i]==1 && BC[i]==-1){
                free=i;
                BC[i]=id+1;
                BT[i]=rtime;
                break;
            }
        }
        pthread_mutex_unlock(&bmtx);
        if(free>=0)break;
        usleep(10000);
    }
    printf("Visitor\t%4d\tFinds boat %4d\n", id+1, free+1);
    minsleep(rtime);
    printf("Visitor\t%4d\tLeaving\n", id+1);

}

void *tboat(void *targ){
    int id = (int)targ;
    printf("Boat\t%4d\tReady\n", id+1);
    while(1){
        V(&rider);
        P(&boat);
        pthread_mutex_lock(&bmtx);
        BA[id]=1;
        BC[id]=-1;
        pthread_barrier_init(&barriers[id], 2);
        pthread_mutex_unlock(&bmtx);
        pthread_barrier_wait(&barriers[id]);
        BA[id]=0;
        printf("Boat\t%4d\tStart of ride for visitor %4d\n", id, BT[id]);
        minsleep(BT[id]);
        printf("Boat\t%4d\tEnd of ride for for visitor %4d (ride time = %d)\n", id, BC[id], BT[id]);
        pthread_barrier_destroy(&barriers[id]);
        pthread_mutex_lock(&bmtx);
        n--;
        if(remainingvistors==0){
            if(remainingboats==1){
                pthread_barrier_wait(&EOS);
            }
            break;
        }
    }
    pthread_barrier_destroy(&barriers[id]);
    exit(0);
}


int main(int argc, char *argv[]) {
    if (argc != 3) {  // Expecting two arguments (program name + 2 integers)
        printf("Error: Please provide exactly two integers.\n");
        printf("Usage: %s <m> <n>\n", argv[0]);
        return 1;  // Exit with error
    }
    srand((unsigned int)time(NULL));

    m = atoi(argv[1]);
    n = atoi(argv[2]);
    remainingvistors=n;
    remainingboats=m;

    pthread_t boats[m], riders[n];

    for (int i = 0; i < m; i++) {
        BA[i] = 0;
        BC[i] = -1;
        BT[i] = 0;
    }

    pthread_barrier_init(&EOS, 2);

    for (int i = 0; i < m; i++) {
        if (pthread_create(&boats[i], NULL, tboat, (void *)(long)i) != 0) {
            perror("Failed to create boat thread");
            exit(1);
        }
    }

    for (int i = 0; i < n; i++) {
        if (pthread_create(&riders[i], NULL, trider, (void *)(long)i) != 0) {
            perror("Failed to create rider thread");
            exit(1);
        }
    }

    for (int i = 0; i < n; i++) {
        pthread_join(riders[i], NULL);
    }

    pthread_mutex_destroy(&bmtx);
    pthread_mutex_destroy(&boat.mtx);
    pthread_mutex_destroy(&rider.mtx);
    pthread_cond_destroy(&boat.cv);
    pthread_cond_destroy(&rider.cv);
}