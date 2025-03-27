/********************
 * LAB ASSIGNMENT 9
 * NAME.: DEVANSHA DHANKER
 * ROLL NO.: 22CS10021 
********************/
#include<iostream>
#include<deque>
#include<array>
#include<fstream>
#include<sstream>
#include<iomanip>
#include<string>
#include<vector>
using namespace std;
int n,m;
vector<vector<int>> process_pair;
vector<int>process_pointer;
vector<vector<short unsigned int>>page_table;
deque<int>ready_que,free_frames,swapped_out_process;
#define MAX_FRAMES 12288
#define ESSENTIAL_SEGMENTS 10
#define VIRTUAL_SEGMENTS 2048
#define VALID_BIT 15
#define PAGE_SIZE 4096
int active_processes,page_accesses,page_faults,swaps,multiprogramming_degree;
int decode_frame(int frame){
    return frame&((1<<VALID_BIT)-1);
}

int encode_frame(int frame){
    return frame|(1<<VALID_BIT);
}

int fetch_frame(int process,int segment,bool bin_search=false){
    if(bin_search) page_accesses++;
    if((page_table[process][segment]>>VALID_BIT)&1){
        return page_table[process][segment];
    }
    if(bin_search) page_faults++;
    if(free_frames.empty()){
        return -1;
    }

    int frame=free_frames.front();
    free_frames.pop_front();
    int ans=(1<<VALID_BIT);
    ans|=frame;
    page_table[process][segment]=ans;
    return frame;
}

int swap_out(int process){
    swaps++;
    active_processes--;
    cout<<"+++ Swapping out process"<<right<<setw(5)<<process<<"  "<<"["<<active_processes<<" active processes"<<"]"<<endl;
    for(int i=0;i<VIRTUAL_SEGMENTS;i++){
        if((page_table[process][i]>>VALID_BIT)&1){
            int frame=decode_frame(page_table[process][i]);
            free_frames.push_back(frame);
        }
        page_table[process][i]&=~(1<<VALID_BIT);
    }
    swapped_out_process.push_back(process);
    return 1;
}

int quit_process(int process){
    active_processes--;
    for(int i=0;i<VIRTUAL_SEGMENTS;i++){
        if((page_table[process][i]>>VALID_BIT)&1){
            int frame=decode_frame(page_table[process][i]);
            free_frames.push_back(frame);
        }
        page_table[process][i]&=~(1<<VALID_BIT);
    }
    return 1;
}

int search(int process,int key){

    #ifdef VERBOSE
    cout<<"    Search "<<process_pointer[process]<<" by Process "<<process<<endl;
    #endif

    int l=0,r=process_pair[process][0]-1;
    // key;
    while(l<r){
        int mid=(l+r)/2;
        int page=ESSENTIAL_SEGMENTS+((mid*4)/PAGE_SIZE);
        int frame=fetch_frame(process,page,true);
        if(frame<0){
            swap_out(process);
            multiprogramming_degree=min(multiprogramming_degree,active_processes);
            return 0;
        }
        if(key>mid) l=mid+1;
        else r=mid;
    }
    return 1;
}

int swap_in(){
    if(swapped_out_process.empty()){
        return 0;
    }
    active_processes++;
    int process=swapped_out_process.front();
    cout<<"+++ Swapping in process"<<right<<setw(6)<<process<<"  "<<"["<<active_processes<<" active processes"<<"]"<<endl;
    swapped_out_process.pop_front();
    for(int i=0;i<ESSENTIAL_SEGMENTS;i++){
        page_table[process][i]=fetch_frame(process,i);
    }
    ready_que.push_front(process);
    return process;
}

int main(){
    ifstream file("search.txt");
    file>>n>>m;
    process_pair.resize(n,vector<int>(m+1,0));
    process_pointer.resize(n,1);
    page_table.resize(n,vector<short unsigned int>(2048,0));
    active_processes=n,page_accesses=0,page_faults=0,swaps=0,multiprogramming_degree=n;
    for(int i=0;i<n;i++){
        int size;
        file>>size;
        process_pair[i][0]=size;
        for(int j=1;j<=m;j++){
            file>>process_pair[i][j];
        }
        
    }
    file.close();
    cout<<"+++ Simulation data read from file"<<endl;

    for(int i=0;i<MAX_FRAMES;i++){
        free_frames.push_back(i);
    }

    for(int i=0;i<n;i++){
        for(int j=0;j<ESSENTIAL_SEGMENTS;j++){
            int frame=fetch_frame(i,j);
            if(frame<0){
                cout<<"Error in allocating frame\n";
                exit(1);
            }
        }
        ready_que.push_back(i);
    }

    cout<<"+++ Kernel data initialized"<<endl;

    while(!ready_que.empty()){
        int process=ready_que.front();
        ready_que.pop_front();
        if(search(process,process_pair[process][process_pointer[process]])){
            process_pointer[process]++;
            if(process_pointer[process]>m){
                quit_process(process);
                swap_in();
            }
            else{
                ready_que.push_back(process);
            }
        }
    }
    cout<<"+++ Page access summary"<<endl;
    cout<<"Total number of page accesses  =  "<<page_accesses<<endl;
    cout<<"Total number of page faults    =  "<<page_faults<<endl;
    cout<<"Total number of swaps          =  "<<swaps<<endl;
    cout<<"Degree of multiprogramming     =  "<<multiprogramming_degree<<endl;
    return 0;
}