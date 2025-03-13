#include <stdio.h>
#include <stdlib.h>
#include <string.h>   
#include <time.h>   
#include <unistd.h>  
#include <pthread.h>  

typedef struct {
    int value;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
} semaphore;

void P(semaphore*s){
    pthread_mutex_lock(&s->mtx);
    while(s->value<=0){
        pthread_cond_wait(&s->cv,&s->mtx);
    }
    s->value--;
    pthread_mutex_unlock(&s->mtx);
}
void V(semaphore*s){
    pthread_mutex_lock(&s->mtx);
    s->value++;
    pthread_cond_signal(&s->cv);
    pthread_mutex_unlock(&s->mtx);
}

semaphore rider = {0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
pthread_mutex_t bmtx = PTHREAD_MUTEX_INITIALIZER;

int m, n;
int *BA, *BC, *BT;
int remainingVisitors, remainingBoats;
pthread_barrier_t *barriers;
pthread_barrier_t EOS;

void minsleep(int min) {
    usleep(min * 100000);  
}

void *trider(void *targ) {
    int id = (int)(long)targ;
    int vtime = 30 + rand() % 91;  // 30 to 120 minutes
    int rtime = 15 + rand() % 46;  // 15 to 60 minutes
    
    printf("Visitor\t%4d\tStarts sightseeing for %3d minutes\n", id+1, vtime);
    minsleep(vtime);
    printf("Visitor\t%4d\tReady to ride a boat (ride time = %d)\n", id+1, rtime);

    P(&rider);
    int free = -1;
    while(1) {   
        pthread_mutex_lock(&bmtx);
        for(int i = 0; i < m; i++) {
            if(BA[i] == 1 && BC[i] == -1) {
                free = i;
                BC[i] = id+1;
                BT[i] = rtime;
                break;
            }
        }
        pthread_mutex_unlock(&bmtx);
        
        if(free >= 0) break;
        usleep(10000);  
    }
    
    printf("Visitor\t%4d\tFinds boat %4d\n", id+1, free+1);
    pthread_barrier_wait(&barriers[free]);  
    minsleep(rtime);
    printf("Visitor\t%4d\tLeaving\n", id+1);
    
    return NULL;
}

void *tboat(void *targ) {
    int id = (int)(long)targ;
    printf("Boat\t%4d\tReady\n", id+1);
    
    while(1) {
        V(&rider);
        pthread_barrier_wait(&barriers[id]);
        
        BA[id] = 0;
        
        int visitor_id = BC[id];
        int rtime = BT[id];
        
        printf("Boat\t%4d\tStart of ride for visitor %4d\n", id+1, visitor_id);
        minsleep(rtime);
        printf("Boat\t%4d\tEnd of ride for visitor %4d (ride time = %d)\n", id+1, visitor_id, rtime);
        
        
        pthread_mutex_lock(&bmtx);
        remainingVisitors--;
        if(remainingVisitors == 0) {
            pthread_mutex_unlock(&bmtx);
            minsleep(10);    // to ensure other threads have completed their execution
            pthread_barrier_wait(&EOS);
            break;
        }
        BA[id] = 1;
        BC[id] = -1;
        pthread_mutex_unlock(&bmtx);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Error: Please provide exactly two integers.\n");
        printf("Usage: %s <m> <n>\n", argv[0]);
        return 1;
    }
    
    srand((unsigned int)time(NULL));

    m = atoi(argv[1]);
    n = atoi(argv[2]);
    
    if (m < 5 || m > 10 || n < 20 || n > 100) {
        printf("Error: m must be between 5 and 10, n must be between 20 and 100\n");
        return 1;
    }
    
    remainingVisitors = n;
    remainingBoats = m;

    BA = (int*)malloc(m * sizeof(int));
    BC = (int*)malloc(m * sizeof(int));
    BT = (int*)malloc(m * sizeof(int));
    barriers = (pthread_barrier_t*)malloc(m * sizeof(pthread_barrier_t));

    pthread_mutex_trylock(&rider.mtx);
    pthread_mutex_unlock(&rider.mtx);
    pthread_mutex_trylock(&bmtx);
    pthread_mutex_unlock(&bmtx);
    
    if (!BA || !BC || !BT || !barriers) {
        perror("Memory allocation failed");
        exit(1);
    }

    pthread_t *boats = (pthread_t*)malloc(m * sizeof(pthread_t));
    pthread_t *riders = (pthread_t*)malloc(n * sizeof(pthread_t));
    
    if (!boats || !riders) {
        perror("Memory allocation failed");
        exit(1);
    }

    for (int i = 0; i < m; i++) {
        BA[i] = 1;
        BC[i] = -1;
        pthread_barrier_init(&barriers[i], NULL, 2);
    }

    pthread_barrier_init(&EOS, NULL, 2);

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

    // Wait for the barrier with the last boat
    pthread_barrier_wait(&EOS);


    pthread_barrier_destroy(&EOS);
    pthread_mutex_destroy(&bmtx);
    pthread_mutex_destroy(&rider.mtx);
    pthread_cond_destroy(&rider.cv);
    
    free(BA);
    free(BC);
    free(BT);
    free(barriers);
    free(boats);
    free(riders);
    
    return 0;
}