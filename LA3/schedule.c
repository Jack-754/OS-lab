#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CPU_MODE 0
#define IO_MODE 1
#define TIMEOUT_MODE 2

#define CPU_FREE 1
#define CPU_OCCUPIED 0

FILE *file;

typedef struct qnode
{
    int idx;
    struct qnode * next;
}qnode;


typedef struct queue
{
    int size;
    qnode * start;
    qnode * end;
}queue;

queue * init(){
    queue * tmp = malloc(sizeof(queue));
    tmp->start=NULL;
    tmp->end=NULL;
    tmp->size=0;
    return tmp;
}

void enqueue(queue * q, int idx){
    qnode * tmp = malloc(sizeof(qnode));
    tmp->idx=idx;
    tmp->next=NULL;
    q->size++;
    if(q->size==1){
        q->start=tmp;
        q->end=tmp;
        return;
    }
    q->end->next=tmp;
    q->end=tmp;
}

void dequeue(queue * q){
    if(q->size==0){
        printf("Dequeue on empty queue.\n");
        exit(1);
    }
    q->size--;
    if(q->size==0){
        free(q->start);
        q->start=NULL;
        q->end=NULL;
        return;
    }
    qnode * tmp = q->start->next;
    free(q->start);
    q->start=tmp;
}

int front(queue *q){
    if(q->size==0){
        printf("Front on empty queue.\n");
        exit(1);
    }
    return q->start->idx;
}

typedef struct PCB{
    int id;
    int arrival;
    int nburst;
    int next;
    int burst[10];
    int io[9];
    int mode;
    int turnaround;
    int total;   
}PCB;

#define MAX_SIZE 10000

PCB * comparator;

typedef struct {
    int data[MAX_SIZE];
    int size;
} MinHeap;

void initHeap(MinHeap* heap) {
    heap->size = 0;
}

int parent(int i) {
    return (i - 1) / 2;
}

int leftChild(int i) {
    return 2 * i + 1;
}

int rightChild(int i) {
    return 2 * i + 2;
}

void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void insert(MinHeap* heap, int value) {
    if (heap->size == MAX_SIZE) {
        printf("Insert in full heap\n");
        exit(1);
    }

    heap->data[heap->size] = value;
    int i = heap->size;
    heap->size++;

    while (i != 0) {
        int parentIndex = parent(i);
        if (comparator[heap->data[parentIndex]].arrival > comparator[heap->data[i]].arrival || 
            (comparator[heap->data[parentIndex]].arrival == comparator[heap->data[i]].arrival && heap->data[parentIndex] > heap->data[i])) {
            swap(&heap->data[parentIndex], &heap->data[i]);
            i = parentIndex;
        } else {
            break;
        }
    }
}

void heapifyDown(MinHeap* heap, int i) {
    int smallest = i;
    int left = leftChild(i);
    int right = rightChild(i);

    // Compare the left child
    if (left < heap->size) {
        if (comparator[heap->data[left]].arrival < comparator[heap->data[smallest]].arrival || 
            (comparator[heap->data[left]].arrival == comparator[heap->data[smallest]].arrival && heap->data[left] < heap->data[smallest])) {
            smallest = left;
        }
    }

    // Compare the right child
    if (right < heap->size) {
        if (comparator[heap->data[right]].arrival < comparator[heap->data[smallest]].arrival || 
            (comparator[heap->data[right]].arrival == comparator[heap->data[smallest]].arrival && heap->data[right] < heap->data[smallest])) {
            smallest = right;
        }
    }

    // Swap and recurse if the smallest is not the current node
    if (smallest != i) {
        swap(&heap->data[i], &heap->data[smallest]);
        heapifyDown(heap, smallest);
    }
}


int extractMin(MinHeap* heap) {
    if (heap->size <= 0) {
        printf("Extract from empty heap\n");
        exit(1);
    }
    if (heap->size == 1) {
        heap->size--;
        return heap->data[0];
    }

    int root = heap->data[0];
    heap->data[0] = heap->data[heap->size - 1];
    heap->size--;
    heapifyDown(heap, 0);

    return root;
}

int min(int a, int b){
    if(a<b)return a;
    return b;
}

void scheduler(int q, PCB proc[], int n){
    comparator=proc;
    MinHeap* event=malloc(sizeof(MinHeap));
    initHeap(event);
    for(int i=0; i<n; i++){
        insert(event, i);
    }
    queue * ready = init();
    int time = 0;
    int STATE = CPU_FREE;

     #ifdef  VERBOSE
        fprintf(file, "%10d: Starting.\n", time);
    #endif
    while(event->size>0 || ready->size>0){
        if(event->size>0){
            int nxt = extractMin(event);
            time = proc[nxt].arrival;
            if(proc[nxt].mode == CPU_MODE){
                if(ready->size==0 )
                #ifdef  VERBOSE
                    fprintf(file, "%10d: Process %d joins ready queue.\n", time, proc[nxt].id);
                #endif
                enqueue(ready, nxt);
            }
            else if(proc[nxt].mode == IO_MODE){
                // expecting IO burst but the last cpu burst was completed
                // printing turnaround time, etc.
                STATE = CPU_FREE;
                if(proc[nxt].nburst-1==proc[nxt].next){
                    proc[nxt].turnaround+=time;
                    int calc=proc[nxt].turnaround/proc[nxt].total;
                    calc*=100;
                    fprintf(file, "%10d :Process %10d exits. Turnaround time = %10d (%4d%%), Wait time = %10d\n", time, proc[nxt].id, proc[nxt].turnaround, calc, proc[nxt].turnaround-proc[nxt].total);
                }
                // performing the next IO burst
                else{
                    proc[nxt].mode = CPU_MODE;
                    proc[nxt].arrival = time + proc[nxt].io[proc[nxt].next];
                    proc[nxt].next++;
                    // #ifdef  VERBOSE
                    //     fprintf(file, "%10d: Process %d joins ready queue after IO completion.\n", time, proc[nxt].id);
                    // #endif
                    insert(event, nxt);
                }
            }
            else if(proc[nxt].mode == TIMEOUT_MODE){
                STATE = CPU_FREE;
                proc[nxt].mode = CPU_MODE;
                insert(event, nxt);
            }
        }

        if(ready->size>0 && STATE == CPU_FREE){
                int nxt=front(ready);
                dequeue(ready);
                STATE = CPU_OCCUPIED;
                if(proc[nxt].burst[proc[nxt].next]<=q){
                    proc[nxt].mode=IO_MODE;
                    proc[nxt].arrival+=min(q, proc[nxt].burst[proc[nxt].next]);
                    #ifdef  VERBOSE
                        fprintf(file, "%10d: Process %d scheduled to run for %d (IO).\n", time, proc[nxt].id, min(q, proc[nxt].burst[proc[nxt].next]));
                    #endif
                    proc[nxt].burst[proc[nxt].next]=0;
                    insert(event, nxt);
                }
                else{
                    proc[nxt].mode=TIMEOUT_MODE;
                    proc[nxt].arrival+=q;
                    proc[nxt].burst[proc[nxt].next]-=q;
                    #ifdef  VERBOSE
                        fprintf(file, "%10d: Process %d scheduled to run for %d  (premepted). \n", time, proc[nxt].id, q);
                    #endif
                    insert(event, nxt);
                }
        }
        else{
            #ifdef  VERBOSE
                fprintf(file, "%10d: CPU goes idle.\n", time);
            #endif
        }
    }

}

void create_copy(PCB proc[], PCB copy[], int n){
    for(int i=0; i<n; i++)copy[i]=proc[i];
}

int main(){
    int n;
    file=fopen("input.txt", "r");
    fscanf(file, "%d", &n);
    PCB proc[n], copy[n];
    for(int i=0; i<n; i++){
        proc[i].nburst=0;
        proc[i].next=0;
        proc[i].mode=CPU_MODE;
        fscanf(file, "%d %d", &proc[i].id, &proc[i].arrival);
        proc[i].turnaround=-proc[i].arrival;
        proc[i].total=0;
        int tmp;
        while(1){
            fscanf(file, "%d", &tmp);
            proc[i].burst[proc[i].nburst++]=tmp;
            proc[i].total+=tmp;
            fscanf(file, "%d", &tmp);
            if(tmp==-1)break;
            proc[i].io[proc[i].nburst-1]=tmp;
            proc[i].total+=tmp;
        }   
    }
    fclose(file);
    int q;
    
    #ifndef VERBOSE
        char * outputfile = "output1.txt";
    #endif
    #ifdef VERBOSE 
        char * outputfile = "verbose_output1.txt";
    #endif

    file=fopen(outputfile, "w");
    fprintf(file, "**** FCFS scheduling ****\n");
    q=1000000000;
    create_copy(proc, copy, n);
    scheduler(q, copy, n);

    // fprintf(file, "\n\n**** RR scheduling with q = 10 ****\n");
    // q=10;
    // create_copy(proc, copy, n);
    // scheduler(q, copy, n);

    // fprintf(file, "\n\n**** RR scheduling with q = 5 ****\n");
    // q=5;
    // create_copy(proc, copy, n);
    // scheduler(q, copy, n);
    
    // NOTE: TO DO RR SCHEDULING WITH A CUSTOM 'q', UNCOMMENT AND ONLY CHANGE VALUE OF VARIABLE 'q' BELOW
    // q=5;     // SET CUSTOM VALUE OF 'q'
    // fprintf(file, "**** RR scheduling with q = %d ****\n", q);
    // create_copy(proc, copy, n);
    // scheduler(q, copy, n);
    fclose(file);


    return 0;
}