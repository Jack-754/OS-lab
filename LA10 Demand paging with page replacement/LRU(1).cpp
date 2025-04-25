#include <iostream>
#include <random>
#include <queue>

using namespace std;

#define VALID_BIT (1U<<15)
#define REFERENCE_BIT (1U<<14)
#define NFFMIN 1000

class Frame {
public:
    int number;
    int pid;
    int page;

    Frame(int num = -1, int p = -1, int pg = -1) : number(num), pid(p), page(pg) {}
};
    
class Node {
public:
    Frame data;
    Node* next;
    Node(Frame val) : data(val), next(nullptr) {}
};

class LinkedList {
private:
    Node* head;
    int size;

public:
    LinkedList() : head(nullptr), size(0) {}

    void insertInOrder(Frame val) {
        size++;
        Node* newNode = new Node(val);
        
        // If list is empty or new frame has smaller number than the head
        if (!head || val.number < head->data.number) {
            newNode->next = head;
            head = newNode;
            return;
        }
        
        // Find the position to insert
        Node* current = head;
        while (current->next && current->next->data.number < val.number) {
            current = current->next;
        }
        
        // Insert new node
        newNode->next = current->next;
        current->next = newNode;
    }

    void deleteValue(Frame val){
        if (!head) return;
        size--;
        if (head->data.number == val.number) {
            Node* temp = head;
            head = head->next;
            delete temp;
            return;
        }
        Node* temp = head;
        while (temp->next && !(temp->next->data.number == val.number)) {
            temp = temp->next;
        }
        if (temp->next) {
            Node* toDelete = temp->next;
            temp->next = temp->next->next;
            delete toDelete;
        }
    }

    void display() {
        Node* temp = head;
        while (temp) {
            cout << "Frame: [Number: " << temp->data.number << ", PID: " << temp->data.pid << ", Page: " << temp->data.page << "] -> ";
            temp = temp->next;
        }
        cout << "NULL\n";
    }

    Node *getHead() {
        return head;
    }

    int getSize() {
        return size;
    }

    ~LinkedList() {
        Node* temp;
        while (head) {
            temp = head;
            head = head->next;
            delete temp;
        }
    }
};
    
typedef struct Process {
    int id;
    int *searchKeys;
    unsigned short pageTable[2048];  // Page table for the process
    unsigned short counter[2048];
    int cur;
    int upperlimit;
    int access;
    int fault;
    int replacement;
    int attempt[4];
}Process;

int n, m;
Process ** proc;

int getPage(int idx){
    return idx/1024 +10;    // 10 added for the first 10 pages to be reserved for the process
}

int pageAvl(int id, int page) {
    return ((proc[id]->pageTable[page]) & VALID_BIT) != 0;
}


int getRandomNumber(int min, int max) {
    random_device rd;  // Seed
    mt19937 gen(rd()); // Mersenne Twister PRNG
    uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

void printStats() {
    printf("+++ Page access summary\n");
    printf("    PID     Accesses        Faults         Replacements                        Attempts\n");
    
    int totalAccess = 0, totalFault = 0, totalReplacement = 0;
    int totalAttempts[4] = {0, 0, 0, 0};
    
    for (int i = 0; i < n; i++) {
        printf("    %-4d     %-6d    %-3d   (%5.2f%%)    %-3d   (%5.2f%%)    %3d + %3d + %3d + %3d  (%5.2f%% + %5.2f%% + %5.2f%% + %5.2f%%)\n",
            i,
            proc[i]->access,
            proc[i]->fault,
            proc[i]->fault * 100.0 / proc[i]->access,
            proc[i]->replacement,
            proc[i]->replacement * 100.0 / proc[i]->access,
            proc[i]->attempt[0], proc[i]->attempt[1], proc[i]->attempt[2], proc[i]->attempt[3],
            proc[i]->replacement > 0 ? proc[i]->attempt[0] * 100.0 / proc[i]->replacement : 0,
            proc[i]->replacement > 0 ? proc[i]->attempt[1] * 100.0 / proc[i]->replacement : 0,
            proc[i]->replacement > 0 ? proc[i]->attempt[2] * 100.0 / proc[i]->replacement : 0,
            proc[i]->replacement > 0 ? proc[i]->attempt[3] * 100.0 / proc[i]->replacement : 0);
            
        totalAccess += proc[i]->access;
        totalFault += proc[i]->fault;
        totalReplacement += proc[i]->replacement;
        
        for (int j = 0; j < 4; j++) {
            totalAttempts[j] += proc[i]->attempt[j];
        }
    }
    
    printf("    Total    %-6d    %-3d   (%5.2f%%)    %-3d   (%5.2f%%)    %3d + %3d + %3d + %3d  (%5.2f%% + %5.2f%% + %5.2f%% + %5.2f%%)\n",
        totalAccess,
        totalFault,
        totalFault * 100.0 / totalAccess,
        totalReplacement,
        totalReplacement * 100.0 / totalAccess,
        totalAttempts[0], totalAttempts[1], totalAttempts[2], totalAttempts[3],
        totalReplacement > 0 ? totalAttempts[0] * 100.0 / totalReplacement : 0,
        totalReplacement > 0 ? totalAttempts[1] * 100.0 / totalReplacement : 0,
        totalReplacement > 0 ? totalAttempts[2] * 100.0 / totalReplacement : 0,
        totalReplacement > 0 ? totalAttempts[3] * 100.0 / totalReplacement : 0);
}

int main(){

    LinkedList FFLIST;

    for(int i=0; i<12288; i++){
        FFLIST.insertInOrder(Frame(i, -1, -1));
    }

    FILE *file= fopen("search.txt", "r");

    fscanf(file, "%d %d", &n, &m);

    proc = new Process*[n];
    queue<int> readyQueue;

    for(int i=0; i<n; i++){
        proc[i] = new Process;
        proc[i]->id = i;
        proc[i]->searchKeys = new int[m];
        proc[i]->cur = 0;
        fscanf(file, "%d", &proc[i]->upperlimit);
        for(int j=0; j<m; j++){
            fscanf(file, "%d", &proc[i]->searchKeys[j]);
        }
        proc[i]->access=0;
        proc[i]->fault=0;
        proc[i]->replacement=0;
        for(int j=0; j<4; j++)proc[i]->attempt[j]=0;
        for(int j=0; j<10; j++){
            Node* head=FFLIST.getHead();
            proc[i]->pageTable[j]=(head->data.number | VALID_BIT | REFERENCE_BIT);
            FFLIST.deleteValue(head->data);
            proc[i]->counter[j]=0xffff;
        }
        readyQueue.push(i);
    }
    printf("+++ Simulation data read from file\n");

    printf("+++ Kernel data initialized\n");

    int remaining = n;
    while(remaining>0){
        int id=readyQueue.front();
        readyQueue.pop();
        int L=0;
        int R=proc[id]->upperlimit-1;
        int key=proc[id]->searchKeys[proc[id]->cur];
        int M;


        #ifdef VERBOSE
        printf("+++ Process %d: Search %d\n", id, proc[id]->cur+1);
        #endif

        while(L<R){
            M=(L+R)/2;
            proc[id]->access++;
            int page=getPage(M);

            if(!pageAvl(id, page)){
                proc[id]->fault++;

                #ifdef VERBOSE
                printf("\tFault on page %d: ", page);
                #endif

                if(FFLIST.getSize()>NFFMIN){
                    Node* head=FFLIST.getHead();

                    #ifdef VERBOSE
                    printf("Free Frame %d found\n", head->data.number);
                    #endif

                    proc[id]->pageTable[page]=head->data.number | VALID_BIT | REFERENCE_BIT;
                    FFLIST.deleteValue(head->data);
                    proc[id]->counter[page]=0xffff;
                }
                else{
                    Node *head=FFLIST.getHead();
                    proc[id]->replacement++;
                    int pageToReplace=-1;
                    for(int i=10; i<2048; i++){
                        if(proc[id]->pageTable[i] & VALID_BIT){
                            if(pageToReplace ==-1  || proc[id]->counter[i]<proc[id]->counter[pageToReplace]){
                                pageToReplace=i;
                            }
                        }
                    }

                    // Capture victim frame before invalidation
                    unsigned short victimEntry = proc[id]->pageTable[pageToReplace];
                    int victimFrame = victimEntry & (~(VALID_BIT | REFERENCE_BIT));
                    proc[id]->pageTable[pageToReplace] = 0; // Invalidate

                    #ifdef VERBOSE
                    printf("To replace Page %d at Frame %d [histor = %d]\n", pageToReplace, victimFrame, proc[id]->counter[pageToReplace]);
                    #endif

                    // CASE 1
                    while(head){
                        if(head->data.pid==id && head->data.page==page){

                            #ifdef VERBOSE
                            printf("\t\tAttempt 1: Page found in free frame %d\n", head->data.number);
                            #endif

                            proc[id]->attempt[0]++;
                            break;
                        }
                        head=head->next;
                    }

                    // CASE 2
                    if(head==nullptr){
                        head=FFLIST.getHead();

                        while(head){
                            if(head->data.pid==-1){

                                #ifdef VERBOSE
                                printf("\t\tAttempt 2: Free frame %d owned by no process found\n", head->data.number);
                                #endif

                                proc[id]->attempt[1]++;
                                break;
                            }
                            head=head->next;
                        }
                    }

                    // CASE 3
                    if(head==nullptr){
                        head=FFLIST.getHead();

                        while(head){
                            if(head->data.pid==id){

                                #ifdef VERBOSE
                                printf("\t\tAttempt 3: Own page %d found in free frame %d\n", head->data.page, head->data.number);
                                #endif

                                proc[id]->attempt[2]++;
                                break;
                            }
                            head=head->next;
                        }

                    }

                    // CASE 4
                    if(head==nullptr){
                        proc[id]->attempt[3]++;
                        head=FFLIST.getHead();
                        int random=getRandomNumber(0, FFLIST.getSize()-1);
                        for(int i=0; i<random; i++){
                            head=head->next;
                        }

                        #ifdef VERBOSE
                        printf("\t\tAttempt 4: Free frame %d owned by Process %d chosen\n", head->data.number, head->data.pid);
                        #endif
                    }

                    // Allocate new frame and update FFLIST
                    proc[id]->pageTable[page] = head->data.number | VALID_BIT | REFERENCE_BIT;
                    FFLIST.deleteValue(head->data);
                    FFLIST.insertInOrder(Frame(victimFrame, id, pageToReplace));
                    proc[id]->counter[page] = 0xffff;
                }
            }
            proc[id]->pageTable[page]=proc[id]->pageTable[page] | REFERENCE_BIT;
            if(key<=M){
                R=M;
            }else{
                L=M+1;
            }
            
        }
        for(int i=10; i<2048; i++){
            if(proc[id]->pageTable[i] & VALID_BIT){
                proc[id]->counter[i]=proc[id]->counter[i]>>1;
                proc[id]->counter[i]=proc[id]->counter[i] | ((proc[id]->pageTable[i] & REFERENCE_BIT)<<1);
                proc[id]->pageTable[i]=(proc[id]->pageTable[i] & ~REFERENCE_BIT);
            }
        }
        proc[id]->cur++;
        if(proc[id]->cur==m){
            proc[id]->cur=0;
            remaining--;
            for (int i = 0; i < 2048; i++) {
                if (proc[id]->pageTable[i] & VALID_BIT) {
                    int frame = proc[id]->pageTable[i] & ~(VALID_BIT | REFERENCE_BIT);
                    FFLIST.insertInOrder(Frame(frame, -1, -1));  // No owner for released frames
                }
            }
        }
        else{
            readyQueue.push(id);
        }
    }

    printStats();

    for (int i = 0; i < n; i++) {
        delete[] proc[i]->searchKeys;
        delete proc[i];
    }
    delete[] proc;
    
    return 0;
}