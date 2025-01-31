#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "boardgen.c"

int A[9][9], S[9][9];
int proc[9];

void send_game(int arr[][2], int flag){
    int saved_stdout = dup(STDOUT_FILENO);

    for(int i=0; i<9; i++){
        int x=(i/3)*3, y=(i%3)*3;

        //code to set stdout to a[i][1]
        dup2(arr[i][1], STDOUT_FILENO);
        printf("n ");
        for(int j=0; j<3; j++){
            y=(i%3)*3;
            for(int k=0; k<3; k++){
                if(flag) printf("%d ", S[x][y]);
                else printf("%d ", A[x][y]);
                y++;
            }
            x++;
        }
        fflush(stdout);
    }

    // code to restore stdout to stdout
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
}   

void put(int fd, int c, int d){
    int saved_stdout = dup(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    printf("p %d %d ", c, d);
    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
}

void quit(int arr[][2]){
    int saved_stdout = dup(STDOUT_FILENO);

    for(int i=0; i<9; i++){
        dup2(arr[i][1], STDOUT_FILENO);
        printf("q ");
        fflush(stdout);
    }
    
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
}

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
            sprintf(position, "17x9+%d+%d", (i%3)*250+1100, (i/3)*250+200);
            execlp("xterm", "xterm", "-T", blockname, "-fa", "Monospace", "-fs", "13", "-geometry", position, "-bg", "#331100",
            "-e", "./block", blockno, bfdin, bfdout, rn1fdout, rn2fdout, cn1fdout, cn2fdout, NULL);
        }
        else{
            proc[i]=p;
        }
        close(arr[i][0]);
    }
    //int debug=0;
    char menu[10];
    printf("Comnmands supported\n");
    printf("\tn      \tStart new game\n");
    printf("\tp b c d\tPut digit d [1-9] at cell c [0-8] of block b [0-8]\n");
    printf("\ts      \tShow solution\n");
    printf("\th      \tPrint this help message\n");
    printf("\tq      \tQuit\n");
    printf("Numbering scheme for blocks and cells\n");
    printf("+---+---+---+\n");
    for(int i=0; i<3; i++){
        printf("| %d ", i);
    }
    printf("|\n");
    printf("+---+---+---+\n");
    for(int i=0; i<3; i++){
        printf("| %d ", i+3);
    }
    printf("|\n");
    printf("+---+---+---+\n");
    for(int i=0; i<3; i++){
        printf("| %d ", i+6);
    }
    printf("|\n");
    printf("+---+---+---+\n");
    printf("\n");

    while(1){
        printf("foodoku> ");
        scanf("%s", menu);
        if(!strcmp(menu, "h")){
            printf("Comnmands supported\n");
            printf("\tn      \tStart new game\n");
            printf("\tp b c d\tPut digit d [1-9] at cell c [0-8] of block b [0-8]\n");
            printf("\ts      \tShow solution\n");
            printf("\th      \tPrint this help message\n");
            printf("\tq      \tQuit\n");
            printf("Numbering scheme for blocks and cells\n");
            printf("+---+---+---+\n");
            for(int i=0; i<3; i++){
                printf("| %d ", i);
            }
            printf("|\n");
            printf("+---+---+---+\n");
            for(int i=0; i<3; i++){
                printf("| %d ", i+3);
            }
            printf("|\n");
            printf("+---+---+---+\n");
            for(int i=0; i<3; i++){
                printf("| %d ", i+6);
            }
            printf("|\n");
            printf("+---+---+---+\n");
        }
        else if(!strcmp(menu, "n")){
            newboard(A, S);
            send_game(arr, 0);
        }
        else if(!strcmp(menu, "p")){
            int b, c, d;
            scanf("%d %d %d", &b, &c, &d);
            if(!(b>=0 && b<=8)){
                printf("b not in valid range [0-8]\n");
                continue;
            }
            else if(!(c>=0 && c<=8)){
                printf("c not in valid range [0-8]\n");
                continue;
            }
            else if(!(d>=1 && d<=9)){
                printf("d not in valid range [1-9]\n");
                continue;
            }
            put(arr[b][1], c, d);
        }
        else if(!strcmp(menu, "s")){
            send_game(arr, 1);
        }
        else if(!strcmp(menu, "q")){
            quit(arr);
            sleep(4);
            for(int i=0; i<9; i++){
                close(arr[i][1]);
            }
            exit(0);
        }

    }
    



}