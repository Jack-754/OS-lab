#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#define DEBUG

// #ifdef DEBUG

// #endif

// current state of a process
#define CPU_MODE 0
#define IO_MODE 1
#define TIMEOUT_MODE 2

//  cpu states
#define CPU_FREE 1        
#define CPU_OCCUPIED 0

FILE *file;

//  structure for nodes stored in queue
typedef struct qnode
{
    int idx;
    struct qnode * next;
}qnode;

//  structures of queue and its related functions
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

// Define the PCB structure
typedef struct PCB {
    int id;
    int arrival;
    int nburst;
    int next;
    int burst[10];
    int io[9];
    int mode;
    int turnaround;
    int total;
} PCB;

// Define constants
#define MAX_SIZE 10000

// Comparator array for accessing PCB objects
PCB* comparator;

// Define the MinHeap structure
typedef struct {
    int data[MAX_SIZE];
    int size;
} MinHeap;

// Function to initialize the heap
void initHeap(MinHeap* heap) {
    heap->size = 0;
}

// Helper functions to calculate parent and children indices
int parent(int i) {
    return (i - 1) / 2;
}

int leftChild(int i) {
    return 2 * i + 1;
}

int rightChild(int i) {
    return 2 * i + 2;
}

// Swap two integers
void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Function to compare two PCBs based on priority rules
int comparePCB(int idx1, int idx2) {
    PCB* pcb1 = &comparator[idx1];
    PCB* pcb2 = &comparator[idx2];

    if (pcb1->arrival != pcb2->arrival) {
        return pcb1->arrival - pcb2->arrival; // Smaller arrival has higher priority
    } else if (pcb1->mode != pcb2->mode) {
        return pcb1->mode - pcb2->mode; // Smaller mode has higher priority
    } else {
        return pcb1->id - pcb2->id; // Smaller ID has higher priority
    }
}

// Insert a value into the heap
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
        if (comparePCB(heap->data[i], heap->data[parentIndex]) < 0) {
            swap(&heap->data[i], &heap->data[parentIndex]);
            i = parentIndex;
        } else {
            break;
        }
    }
}

// Heapify down to maintain heap property
void heapifyDown(MinHeap* heap, int i) {
    int smallest = i;
    int left = leftChild(i);
    int right = rightChild(i);

    // Compare left child
    if (left < heap->size && comparePCB(heap->data[left], heap->data[smallest]) < 0) {
        smallest = left;
    }

    // Compare right child
    if (right < heap->size && comparePCB(heap->data[right], heap->data[smallest]) < 0) {
        smallest = right;
    }

    // Swap and recurse if the smallest is not the current node
    if (smallest != i) {
        swap(&heap->data[i], &heap->data[smallest]);
        heapifyDown(heap, smallest);
    }
}

// Extract the minimum value from the heap
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

//  print heap contents
void printHeap(MinHeap* heap) {
    printf("Heap Contents (Size: %d):\n", heap->size);
    
    if (heap->size == 0) {
        printf("  Heap is empty\n");
        return;
    }

    printf("  Index\tProcess ID\tArrival Time\n");
    printf("  -----\t----------\t------------\n");

    for (int i = 0; i < heap->size; i++) {
        int processIndex = heap->data[i];
        printf("  %d\t%d\t\t%d\t%d\n", 
            i,                                  // Heap index
            comparator[processIndex].id,        // Process ID
            comparator[processIndex].arrival,    // Arrival Time
            comparator[processIndex].mode       // Current state of process
        );
    }
    printf("\n");
}

// roundoff to nearest integer
int roundoff(float num) {
    int int_part = (int)num;
    if (num - int_part >= 0.5) {
        return int_part + 1;
    } else if (num - int_part <= -0.5) {
        return int_part - 1;
    }
    return int_part;
}

// scheduling algorithm
void scheduler(int q, PCB proc[], int n){
    comparator=proc;
    MinHeap* event=malloc(sizeof(MinHeap));
    initHeap(event);
    for(int i=0; i<n; i++){
        insert(event, i);
    }
    queue * ready = init();
    int time = 0;
    int STATE = CPU_FREE, EXECUTING;
    int total_wait=0, total_idle=0, flag=0;
     #ifdef  VERBOSE
        fprintf(file, "%-10d: Starting\n", time);
    #endif
    while(event->size>0 || ready->size>0){
        if(event->size>0){
            int nxt = extractMin(event);

            time = proc[nxt].arrival;

            if(proc[nxt].mode == CPU_MODE){
                #ifdef  VERBOSE
                if(proc[nxt].turnaround==-proc[nxt].arrival) fprintf(file, "%-10d: Process %d joins ready queue upon arrival\n", time, proc[nxt].id);
                else fprintf(file, "%-10d: Process %d joins ready queue after IO completion\n", time, proc[nxt].id);
                #endif
                enqueue(ready, nxt);
            }
            else if(proc[nxt].mode == IO_MODE){

                if(proc[nxt].id == EXECUTING){
                    STATE = CPU_FREE;
                }

                // expecting IO burst but the last cpu burst was completed
                // printing turnaround time, etc.
                if(proc[nxt].nburst-1==proc[nxt].next){
                    proc[nxt].turnaround+=time;
                    int calc=roundoff((((float)proc[nxt].turnaround)/proc[nxt].total)*100);
                    total_wait+=proc[nxt].turnaround-proc[nxt].total;
                    fprintf(file, "%-10d: Process %6d exits. Turnaround time =%5d (%3d%%), Wait time = %d\n", time, proc[nxt].id, proc[nxt].turnaround, calc, proc[nxt].turnaround-proc[nxt].total);
                }

                // performing the next IO burst
                else{
                    proc[nxt].mode = CPU_MODE;
                    proc[nxt].arrival = time + proc[nxt].io[proc[nxt].next];
                    proc[nxt].next++;
                    insert(event, nxt);
                }
            }
            else if(proc[nxt].mode == TIMEOUT_MODE){
                STATE = CPU_FREE;
                proc[nxt].mode = CPU_MODE;
                #ifdef  VERBOSE
                    fprintf(file, "%-10d: Process %d joins ready queue after timeout\n", time, proc[nxt].id);
                #endif
                enqueue(ready, nxt);
            }
        }

        if(ready->size>0 && STATE == CPU_FREE){
                int nxt=front(ready);
                dequeue(ready);
                STATE = CPU_OCCUPIED;
                EXECUTING = proc[nxt].id;
                if(flag==1){
                    total_idle+=time;
                    flag=0;
                }
                if(proc[nxt].burst[proc[nxt].next]<=q){
                    proc[nxt].mode=IO_MODE;
                    proc[nxt].arrival=time+min(q, proc[nxt].burst[proc[nxt].next]);
                    #ifdef  VERBOSE
                        fprintf(file, "%-10d: Process %d is scheduled to run for time %d\n", time, proc[nxt].id, min(q, proc[nxt].burst[proc[nxt].next]));
                    #endif
                    proc[nxt].burst[proc[nxt].next]=0;
                    insert(event, nxt);
                }
                else{
                    proc[nxt].mode=TIMEOUT_MODE;
                    proc[nxt].arrival=time+q;
                    proc[nxt].burst[proc[nxt].next]-=q;
                    #ifdef  VERBOSE
                        fprintf(file, "%-10d: Process %d is scheduled to run for time %d\n", time, proc[nxt].id, q);
                    #endif
                    insert(event, nxt);
                }
        }
        else if(STATE == CPU_FREE){
            if(flag==0){
                total_idle-=time;
                flag=1;
            }
            #ifdef  VERBOSE
                fprintf(file, "%-10d: CPU goes idle\n", time);
            #endif
        }
    }
    if(flag==1){
        total_idle+=time;
    }
    fprintf(file, "\nAverage wait time = %.2f\n", ((float)total_wait)/n);
    fprintf(file, "Total turnaround time = %d\n", time);
    fprintf(file, "CPU idle time = %d\n", total_idle);
    fprintf(file, "CPU utilization = %.2f%%\n", (((float)(time-total_idle))/time)*100);

}

void create_copy(PCB proc[], PCB copy[], int n){
    for(int i=0; i<n; i++)copy[i]=proc[i];
}

int main(){
    int n;
    char * inputfile = "proc.txt";  // set input file name here
    printf("Reading file %s\n", inputfile);
    file=fopen(inputfile, "r");

    if(file == NULL){
        printf("No input file named %s found. Terminating...", inputfile);
        exit(1);
    }

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
        char * outputfile = "output.txt";
    #endif
    #ifdef VERBOSE 
        char * outputfile = "verbose_output.txt";
    #endif

    file=fopen(outputfile, "w");

    fprintf(file, "**** FCFS Scheduling ****\n");
    q=1000000000;
    create_copy(proc, copy, n);
    scheduler(q, copy, n);

    fprintf(file, "\n**** RR Scheduling with q = 10 ****\n");
    q=10;
    create_copy(proc, copy, n);
    scheduler(q, copy, n);

    fprintf(file, "\n**** RR Scheduling with q = 5 ****\n");
    q=5;
    create_copy(proc, copy, n);
    scheduler(q, copy, n);
    
    // NOTE: TO DO RR SCHEDULING WITH A CUSTOM 'q', UNCOMMENT AND ONLY CHANGE VALUE OF VARIABLE 'q' BELOW
    // q=5;     // SET CUSTOM VALUE OF 'q'
    // fprintf(file, "**** RR scheduling with q = %d ****\n", q);
    // create_copy(proc, copy, n);
    // scheduler(q, copy, n);

    
    printf("Output saved to %s\n", outputfile);
    fclose(file);


    return 0;
}