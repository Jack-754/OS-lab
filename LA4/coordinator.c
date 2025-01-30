#include <stdio.h>
#include <unistd.h>
#include "boardgen.c"

int main(){
    
    int arr[9][2];
    for(int i=0; i<9; i++){
        pipe(arr[i]);
    }
    for(int i=0; i<9; i++){
        int p=fork();
        if(!p){
            char blockname[10], blockno[10], bfdin[10], bfdout[10], rn1fdout[10], rn2fdout[10], cn1fdout[10], cn2fdout[10], position[20];
            sprintf(blockname, "Block %d", i);
            sprintf(blockno, "%d", i);
            sprintf(bfdin, "%d", arr[i][0]);
            sprintf(bfdout, "%d", arr[i][1]);
            sprintf(cn1fdout, "%d", arr[(i+3)%9][1]);
            sprintf(cn2fdout, "%d", arr[(i+6)%9][1]);
            int nrows[2];
            int cnt=0;
            int tmp=(i/3)*3;
            for(int j=0; j<3; j++){
                if(i!=tmp)nrows[cnt++]=tmp;
                tmp++;
            }
            sprintf(rn1fdout, "%d", arr[nrows[0]][1]);
            sprintf(rn2fdout, "%d", arr[nrows[1]][1]);
            sprintf(position, "20x8+%d+%d", (i%3)*250+1100, (i/3)*250+200);
            execlp("xterm", "xterm", "-T", blockname, "-fa", "Monospace", "-fs", "15", "-geometry", position, "-bg", "#331100",
            "-e", "./block", blockno, bfdin, bfdout, rn1fdout, rn2fdout, cn1fdout, cn2fdout, NULL);
        }
    }

    



}