#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int  blockno, bfdin, bfdout, rn1fdout, rn2fdout, cn1fdout, cn2fdout;

int request_check(int fd, int i, int d, char type, int reply) {
    int saved = dup(STDOUT_FILENO);  // Save original stdout
    if (saved == -1) {
        perror("dup request_check 0");
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) {  // Redirect stdout to fd
        perror("dup2 request_check 1");
        close(saved);
        return -1;
    }
    printf("%c %d %d %d ", type, i, d, reply);
    fflush(stdout);  // Ensure output is written immediately
    if (dup2(saved, STDOUT_FILENO) == -1) {  // Restore original stdout
        perror("dup2 request_check 2");
        close(saved);
        return -1;
    }
    close(saved);  // Close the saved file descriptor
    int success;
    if (scanf("%d", &success) != 1) {  // Ensure scanf reads an integer
        perror("scanf request_check");
        return -1;
    }
    return success;
}

void reply_check(char type, int i, int d, int fd, int B[][3]){
    int saved = dup(STDOUT_FILENO);  // Save original stdout
    if (saved == -1) {
        perror("dup reply_check 0");
        return;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) {  // Redirect stdout to fd
        perror("dup2 reply_check 1");
        close(saved);
        return;
    }
    int flag=0;
    if(type=='r'){
        for(int j=0; j<3; j++){
            if(B[i][j]==d){
                flag=1;
                break;
            }
        }
    }
    else{
        for(int j=0; j<3; j++){
            if(B[j][i]==d){
                flag=1;
                break;
            }
        }
    }
    printf("%d ", flag);
    fflush(stdout);  // Ensure output is written immediately
    if (dup2(saved, STDOUT_FILENO) == -1) {  // Restore original stdout
        perror("dup2 reply_check 2");
        close(saved);
        return;
    }
    close(saved);  // Close the saved file descriptor
}

int already_used(int B[][3], int c, int d){
    for(int i=0; i<3; i++){
        for(int j=0; j<3; j++){
            if(c/3==i && c%3==j)continue;
            if(B[i][j]==d)return 1;
        }
    }
    return 0;
}

void print_board(int B[][3]){
    system("clear");
    printf("+---+---+---+\n");
    for(int i=0; i<3; i++){
        if(B[0][i]==0)printf("|   ");
        else printf("| %d ", B[0][i]);
    }
    printf("|\n");
    printf("+---+---+---+\n");
    for(int i=0; i<3; i++){
        if(B[1][i]==0)printf("|   ");
        else printf("| %d ", B[1][i]);
    }
    printf("|\n");
    printf("+---+---+---+\n");
    for(int i=0; i<3; i++){
        if(B[2][i]==0)printf("|   ");
        else printf("| %d ", B[2][i]);
    }
    printf("|\n");
    printf("+---+---+---+\n");
}

int main(int argc, char *argv[]){
    blockno=atoi(argv[1]);
    bfdin=atoi(argv[2]);  //read end
    bfdout=atoi(argv[3]); //write end
    rn1fdout=atoi(argv[4]);
    rn2fdout=atoi(argv[5]);
    cn1fdout=atoi(argv[6]);
    cn2fdout=atoi(argv[7]);


    close(0);
    dup(bfdin);
    int A[3][3], B[3][3];
    char menu[10];

    printf("Block %d ready\n", blockno);

    while(1){
        scanf("%s", menu);
        if(!strcmp(menu, "n")){
            for(int i=0; i<3; i++){
                for(int j=0; j<3; j++){
                    scanf("%d", &A[i][j]);
                    B[i][j]=A[i][j];
                }
            }
            print_board(B);     
        }
        else if(!strcmp(menu, "p")){
            int c, d;
            scanf("%d %d", &c, &d);
            print_board(B);
            if(A[c/3][c%3]){
                printf("Read-only Cell\n");
                fflush(stdout);
            }
            else if(already_used(B, c, d)){
                printf("Block conflict\n");
                fflush(stdout);
            }
            else{
                int flag;
                flag=request_check(rn1fdout, c/3, d, 'r', bfdout);
                if(flag==1){
                    printf("Row conflict\n");
                    fflush(stdout);
                    continue;
                }
                flag=request_check(rn2fdout, c/3, d, 'r', bfdout);
                if(flag==1){
                    printf("Row conflict\n");
                    fflush(stdout);
                    continue;
                }
                flag=request_check(cn1fdout, c%3, d, 'c', bfdout);
                if(flag==1){
                    printf("Column conflict\n");
                    fflush(stdout);
                    continue;
                }
                flag=request_check(cn2fdout, c/3, d, 'c', bfdout);
                if(flag==1){
                    printf("Column conflict\n");
                    fflush(stdout);
                    continue;
                }
                B[c/3][c%3]=d;
                print_board(B);
            }
        }
        else if(!strcmp(menu, "r") || !strcmp(menu, "c")){
            int i, d, fd;
            scanf("%d %d %d", &i, &d, &fd);
            reply_check(menu[0], i, d, fd, B);
        }
        else if(!strcmp(menu, "q")){
            print_board(B);
            printf("Bye\n");
            fflush(stdout);
            sleep(2);
            exit(0);
        }
    }

}