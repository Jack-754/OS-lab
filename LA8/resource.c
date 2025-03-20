#include <stdio.h>
#include <stdlib.h>
#include <string.h>   
#include <time.h>   
#include <unistd.h>  
#include <pthread.h>
#include <signal.h>

#define RELEASE 0
#define ADDITIONAL 1

int n, m, threads_alive;
int ** need, ** alloc, * available;
int reqtype, reqfrom, *req;
pthread_mutex_t rmtx, pmtx, *cmtx;
pthread_cond_t *cv;
pthread_barrier_t BOS, REQB, *ACKB;

void minsleep(int min) {
    usleep(min * 100000);  
}

typedef struct Node {
    int id;
    int *req;
    struct Node* next;
} Node;

typedef struct Queue {
    int size;
    Node* front;
    Node* rear;
} Queue;

void freeResources(){
    pthread_mutex_destroy(&rmtx);
    pthread_mutex_destroy(&pmtx);
    for(int i=0; i<n; i++){
        pthread_mutex_destroy(cmtx+i);
        pthread_cond_destroy(cv+i);
        pthread_barrier_destroy(ACKB+i);
        free(need[i]);
        free(alloc[i]);
    }
    pthread_barrier_destroy(&BOS);
    pthread_barrier_destroy(&REQB);
    free(cv);
    free(cmtx);
    free(ACKB);
    free(need);
    free(alloc);
    free(available);
    free(req);
}

void signalhandler(int signum){
    freeResources();
    exit(0);
}

Node* createNode(int id, int req[]){
    Node *node=malloc(sizeof(Node));
    node->id=id;
    node->req=malloc(m*sizeof(int));
    for(int i=0; i<m; i++)node->req[i]=req[i];
    node->next=NULL;
    return node;
}

void freeNode(Node* node){
    free(node->req);
    free(node);
}

Queue* createQueue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    q->size=0;
    return q;
}

void enqueue(Queue* q, Node *newNode) {
    q->size++;
    if (q->front == NULL) {
        q->front = q->rear = newNode;
        return;
    }
    q->rear->next = newNode;
    q->rear = newNode;
}

Node* dequeue(Queue* q) {
    if (q->front == NULL) {
        printf("Queue is empty!\n");
        return NULL;
    }
    q->size--;
    Node* temp = q->front;
    q->front = temp->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    return temp;
}

int isEmpty(Queue* q) {
    return q->front == NULL;
}

void *tmain(void *targ){
    int id = (int)(long)targ;
    pthread_mutex_lock(&pmtx);
    printf("Thread %d born\n", id);
    pthread_mutex_unlock(&pmtx);
    char filename[25];
    sprintf(filename, "input/thread%02d.txt", id);
    FILE*fp=fopen(filename, "r");
    if(fp==NULL){
        printf("Unable to open file %s\n", filename);
        pthread_exit(NULL);
    }
    for(int i=0; i<m; i++){
        fscanf(fp, "%d", need[id]+i);
        alloc[id][i]=0;
    }

    pthread_barrier_wait(&BOS);
    int delay;
    char action[2];

    while(1){
        fscanf(fp, "%d %s", &delay, action);

        minsleep(delay);

        if(!strcmp(action, "Q")){
            pthread_mutex_lock(&rmtx);
            reqtype=RELEASE;
            reqfrom=id;
            for(int i=0; i<m; i++){
                req[i]=-alloc[id][i];
            }
            threads_alive--;
            pthread_barrier_wait(ACKB+id);
            pthread_mutex_unlock(&rmtx);
            pthread_exit(NULL);
            break;
        }
        
        pthread_mutex_lock(&rmtx);
        reqtype=RELEASE;
        reqfrom=id;
        for(int i=0; i<m; i++){
            fscanf(fp, "%d", req+i);
            if(req[i]>0)reqtype=ADDITIONAL;
        }

        pthread_mutex_lock(&pmtx);
        printf("\tThread %d sends resource request: type = ", id);
        if(reqtype==ADDITIONAL){
            printf("ADDITIONAL\n");
        }
        else printf("RELEASE\n");
        pthread_mutex_unlock(&pmtx);

        pthread_barrier_wait(&REQB);
        pthread_barrier_wait(ACKB+id);
        if(reqtype==RELEASE){
            pthread_mutex_unlock(&rmtx);

            pthread_mutex_lock(&pmtx);
            printf("\tThread %d is granted its last resource request\n", id);
            pthread_mutex_unlock(&pmtx);
            continue;
        }
        pthread_mutex_unlock(&rmtx);

        pthread_mutex_lock(cmtx+id);
        pthread_cond_wait(cv+id, cmtx+id);
        pthread_mutex_unlock(cmtx+id);
        printf("\t Thread %d is granted its last resource request\n", id);
    }


}

void create_mutex (){
    cmtx = (pthread_mutex_t*)malloc(n*sizeof(pthread_mutex_t));
    cv = (pthread_cond_t*)malloc(n*sizeof(pthread_cond_t));

    pthread_mutex_init(&rmtx, NULL);
    pthread_mutex_trylock(&rmtx);
    pthread_mutex_unlock(&rmtx);

    pthread_mutex_init(&pmtx, NULL);
    pthread_mutex_trylock(&pmtx);
    pthread_mutex_unlock(&pmtx);

    for(int i=0; i<n; i++){
        pthread_mutex_init(cmtx+i, NULL);
        pthread_mutex_trylock(cmtx+i);
        pthread_mutex_unlock(cmtx+i);
        pthread_cond_init(cv+i, NULL);
    }
}

void create_barriers(){
    ACKB = (pthread_barrier_t*)malloc(n*sizeof(pthread_barrier_t));
    pthread_barrier_init(&BOS, NULL, n+1);
    pthread_barrier_init(&REQB, NULL, 2);
    for(int i=0; i<n; i++)pthread_barrier_init(ACKB+i, NULL, 2);
}

void create_threads(pthread_t* tid){
    pthread_attr_t attr;
    /* Set attributes for creating joinable threads */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(int i=0; i<n; i++){
        if  (pthread_create(tid + i, &attr, tmain, (void *)(long)i)) {
            fprintf(stderr, "Master thread: Unable to create thread\n");
            pthread_attr_destroy(&attr);
            exit(1);
        }
    }

    sleep(1);
    pthread_attr_destroy(&attr);
}

int isSafeState() {
    #ifndef _DLAVOID
    return 1;
    #endif

    int finish[n];
    for (int i = 0; i < n; i++) finish[i] = 0;
    int work[m];
    for (int j = 0; j < m; j++) work[j] = available[j];

    int count = 0;
    while (count < n) {
        int found = 0;
        for (int i = 0; i < n; i++) {
            if (!finish[i]) {
                int canExecute = 1;
                for (int j = 0; j < m; j++) {
                    if (need[i][j] > work[j]) {
                        canExecute = 0;
                        break;
                    }
                }
                if (canExecute) {
                    for (int j = 0; j < m; j++) work[j] += alloc[i][j];
                    finish[i] = 1;
                    count++;
                    found = 1;
                }
            }
        }
        if (!found) return 0;
    }
    return 1;
}

int canGrantRequest(int req[]) {
    for (int j = 0; j < m; j++) {
        if (req[j] > available[j]) {
            return 0; // Request exceeds available resources
        }
    }
    return 1;
}

void printArray(int arr[], int length) {
    for (int i = 0; i < length; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(){
    FILE * file=fopen("input/system.txt", "r");
    if(file==NULL){
        perror("Can't open the file input/system.txt\n");
        exit(0);
    }

    fscanf(file, "%d %d", &m, &n);
    
    threads_alive=n;

    available=malloc(m*sizeof(int));
    req=malloc(m*sizeof(int));
    need=malloc(n*sizeof(int*));
    alloc=malloc(n*sizeof(int*));

    for(int i=0; i<n; i++){
        need[i]=malloc(m*sizeof(int));
        alloc[i]=malloc(m*sizeof(int));
    }

    for(int i=0; i<m; i++){
        fscanf(file, "%d", &available[i]);
    }

    create_mutex();
    create_barriers();

    pthread_t tid[n];
    create_threads(tid);
    
    pthread_barrier_wait(&BOS);

    signal(SIGINT, signalhandler);

    Queue* queue=createQueue();

    while(threads_alive){
        pthread_barrier_wait(&REQB);
        if(reqtype==RELEASE){
            for(int i=0; i<m; i++){
                alloc[reqfrom][i]+=req[i];
                available[i]-=req[i];
            }
            pthread_barrier_wait(ACKB+reqfrom);
        }
        else{
            for(int i=0; i<m; i++){
                if(req[i]<0){
                    alloc[reqfrom][i]+=req[i];
                    need[reqfrom][i]-=req[i];
                    available[i]-=req[i];
                    req[i]=0;
                }
            }
            pthread_mutex_lock(&pmtx);
            printf("Master thread stores resource request of thread %d\n", reqfrom);
            pthread_mutex_unlock(&pmtx);

            enqueue(queue, createNode(reqfrom, req));
            pthread_barrier_wait(ACKB+reqfrom);
        }
        


        pthread_mutex_lock(&pmtx);

        printf("\t\tWaiting threads: ");
        for(int i=0; i<queue->size; i++){
            Node *node=dequeue(queue);
            printf("%d ", node->id);
            enqueue(queue, node);
        }
        printf("\n");

        printf("Master thread tries to grant pending requests\n");
        int cur_que_size=queue->size;
        for(int i=0; i<cur_que_size; i++){
            Node *node=dequeue(queue);
            if(canGrantRequest(node->req)){
                for(int i=0; i<m; i++){
                available[i]-=node->req[i];
                alloc[node->id][i]+=node->req[i];
                need[node->id][i]-=node->req[i];
                }
                if(isSafeState()){
                    usleep(10000);  // to ensure the thread reached conditional wait
                    pthread_mutex_lock(cmtx+node->id);
                    pthread_cond_signal(cv+node->id);
                    pthread_mutex_unlock(cmtx+node->id);
                    printf("Master thread grants resource request for thread %d\n", node->id);
                    free(node->req);
                    free(node);
                }
                else{
                    printf("\t+++ Insufficient resources to grant request of thread %d\n", node->id);
                    for(int i=0; i<m; i++){
                        available[i]+=node->req[i];
                        alloc[node->id][i]-=node->req[i];
                        need[node->id][i]+=node->req[i];
                    }
                    enqueue(queue, node);
                }
            }
            else{  
                printf("\t+++ Insufficient resources to grant request of thread %d\n", node->id);
                enqueue(queue, node);
            }
        }

        printf("\t\tWaiting threads: ");
        for(int i=0; i<queue->size; i++){
            Node *node=dequeue(queue);
            printf("%d ", node->id);
            enqueue(queue, node);
        }
        printf("\n");

        pthread_mutex_unlock(&pmtx);
    }

    for(int i=0; i<n; i++){
        if (pthread_join(tid[i],NULL)) {
            fprintf(stderr, "Unable to wait for thread [%lu]\n", tid[i]);
         } else {
            printf("%d has joined\n", i);
         }
    }
    free(queue);
    freeResources();
}