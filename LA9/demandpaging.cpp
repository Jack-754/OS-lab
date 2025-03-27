#include <iostream>
#include <queue>


#define VALID (1U<<15)

using namespace std;

typedef struct Process {
    int id;
    int arraySize;
    int *searchKeys;
    unsigned short pageTable[2048];  // Page table for the process
    int cur;
    int L;
    int R;
    int upperlimit;
}Process;

queue<int> readyQueue;
queue<int> swappedOut;
queue<unsigned short> freeFrames;

int n;
int m;
int pageAcc = 0;
int pageFlt = 0;
int swapOps = 0;
int active = 0;
int minAct = (int)1e9;

Process **processes;

int getPage(int idx){
    return idx/1024 +10;    // 10 added for the first 10 pages to be reserved for the process
}

int pageAvl(Process *proc, int page) {
    return ((proc->pageTable[page]) & VALID) != 0;
}

void freePage(int id){
    Process *proc = processes[id];
    for(int i=0; i<2048; i++){
        if(proc->pageTable[i] & VALID){
            freeFrames.push(proc->pageTable[i] ^ VALID);
            proc->pageTable[i] = 0;
        }
    }
}

void swapOutProcess(int id) {
    Process *proc = processes[id];
    swappedOut.push(proc->id);
    freePage(id);
    swapOps++;

    minAct = min(minAct, (int)readyQueue.size());

    printf("+++ Swapping out process %4d  [%d active processes]\n", id, (int)readyQueue.size());
}

int loadPage(int id, int page){
    Process *proc = processes[id];

    if (freeFrames.empty()) {
        return 0;
    }

    // Allocate a free frame
    unsigned short frame = freeFrames.front();
    freeFrames.pop();

    // Update page table
    proc->pageTable[page] = VALID | frame;

    return 1;
}

int main(){
    int n, m;

    FILE *file= fopen("search.txt", "r");

    fscanf(file, "%d %d", &n, &m);

    processes = new Process*[n];
    for(int i=0; i<n; i++){
        processes[i] = new Process;
        processes[i]->id = i;
        processes[i]->arraySize = m;
        processes[i]->searchKeys = new int[m];
        processes[i]->cur = 0;
        processes[i]->L = 0;
        fscanf(file, "%d", &processes[i]->upperlimit);
        processes[i]->R = processes[i]->upperlimit - 1;
        for(int j=0; j<m; j++){
            fscanf(file, "%d", &processes[i]->searchKeys[j]);
        }

        for(int j=0; j<2048; j++){
            processes[i]->pageTable[j] = 0;
        }

        readyQueue.push(i);

    }

    printf("+++ Simulation data read from file\n");

    for(unsigned short int i=0; i<12288; i++){
        freeFrames.push(i);
    }

    printf("+++ Kernel data initialized\n");

    for(int i=0; i<n; i++){
        for(int j=0; j<10; j++){
            processes[i]->pageTable[j] = VALID | freeFrames.front();
            freeFrames.pop();
        }
    }

    int id=readyQueue.front();
    readyQueue.pop();
    Process *p=processes[id];

    int remaining = n;

    while(remaining > 0){
        int L=p->L;
        int R=p->R;
        int key=p->searchKeys[p->cur];
        int M;

        int flag = 1;

        #ifdef VERBOSE
        printf("\tSearch %d by Process %d\n", p->cur+1, id);
        #endif

        while(L<R){
            M=(L+R)/2;
            pageAcc++;
            int page=getPage(M);
            if(pageAvl(p, page)){
                if(key<=M){
                    R=M;
                }else{
                    L=M+1;
                }
            }
            else{
                pageFlt++;
                if(loadPage(id, page)){
                    if(key<=M){
                        R=M;
                    }else{
                        L=M+1;
                    }
                }
                else{
                    // p->L=L;
                    // p->R=R;
                    swapOutProcess(id);
                    id = readyQueue.front();
                    readyQueue.pop();
                    p=processes[id];
                    L=p->L;
                    R=p->R;
                    key=p->searchKeys[p->cur];
                    flag=0;
                    break;
                }
            }
        }

        if(flag){
            p->cur++;
            if(p->cur>=m){   
                remaining--; 
                if(remaining==0)break;    
                freePage(id);
                if(swappedOut.size() > 0){
                    id = swappedOut.front();
                    swappedOut.pop();
                    p=processes[id];
                    L=p->L;
                    R=p->R;
                    key=p->searchKeys[p->cur];
                    for(int i=0; i<10; i++){
                        p->pageTable[i] = (VALID | freeFrames.front());
                        freeFrames.pop();
                    }
                    printf("+++ Swapping in process  %4d  [%d active processes]\n", id, (int)readyQueue.size()+1);
                }
                else{
                    id = readyQueue.front();
                    readyQueue.pop();
                    p=processes[id];
                    L=p->L;
                    R=p->R;
                    key=p->searchKeys[p->cur];
                }
            }
            else{
                L=0;
                R=p->upperlimit-1;
                p->L=L;
                p->R=R;
                readyQueue.push(id);
                id = readyQueue.front();
                readyQueue.pop();
                p=processes[id];
                L=p->L;
                R=p->R;
                key=p->searchKeys[p->cur];
            }
        }
        
    }

    printf("+++ Page access summary\n");
    printf("\tTotal number of page accesses  =  %d\n", pageAcc);
    printf("\tTotal number of page faults    =  %d\n", pageFlt);
    printf("\tTotal number of swaps          =  %d\n", swapOps);
    printf("\tDegree of multiprogramming     =  %d\n", minAct);

    return 0;
}