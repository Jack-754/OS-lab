#include <stdio.h>
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
    printf("%c %d %d %d\n", type, i, d, reply);
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
    printf("%d", flag);
    fflush(stdout);  // Ensure output is written immediately
    if (dup2(saved, STDOUT_FILENO) == -1) {  // Restore original stdout
        perror("dup2 reply_check 2");
        close(saved);
        return;
    }
    close(saved);  // Close the saved file descriptor
}

int already_used(int B[][3], int d){
    for(int i=0; i<3; i++){
        for(int j=0; j<3; j++){
            if(B[i][j]==d)return 1;
        }
    }
    return 0;
}

void print_board(int B[][3]){
    printf("+---+---+---+\n");
    for(int i=0; i<3; i++){
        if(B[0][i]==0)printf("|   ");
        else printf("| %d ");
    }
    printf("|\n");
    printf("+---+---+---+\n");
    for(int i=0; i<3; i++){
        if(B[1][i]==0)printf("|   ");
        else printf("| %d ");
    }
    printf("|\n");
    printf("+---+---+---+\n");
    for(int i=0; i<3; i++){
        if(B[2][i]==0)printf("|   ");
        else printf("| %d ");
    }
    printf("|\n");
    printf("+---+---+---+\n");
}


int main(int argc, char *argv[]){
    char * endptr;
    blockno=strtol(argv[1], endptr, 10);
    bfdin=strtol(argv[2], endptr, 10);  //read end
    bfdout=strtol(argv[3], endptr, 10); //write end
    rn1fdout=strtol(argv[4], endptr, 10);
    rn2fdout=strtol(argv[5], endptr, 10);
    cn1fdout=strtol(argv[6], endptr, 10);
    cn2fdout=strtol(argv[7], endptr, 10);

    for(int i=0; i<argc; i++){
        printf("%s\n", argv[i]);
    }
    sleep(10);

    close(0);
    dup(bfdin);
    int A[3][3], B[3][3];
    char menu;
    while(1){
        scanf("%c", &menu);
        if(menu=='n'){
            for(int i=0; i<3; i++){
                for(int j=0; j<3; j++){
                    scanf("%d", &A[i][j]);
                    B[i][j]=A[i][j];
                }
            }
            system("clear");
            print_board(B);
        }
        else if(menu=='p'){
            int c, d;
            scanf("%d %d", &c, &d);
            if(A[c/3][c%3]){
                printf("Read-only Cell\n");
            }
            else if(already_used(B, d)){
                printf("Block conflict\n");
            }
            else{
                int flag;
                flag=request_check(rn1fdout, c/3, d, 'r', bfdout);
                if(flag==1){
                    printf("Row conflict\n");
                    continue;
                }
                flag=request_check(rn2fdout, c/3, d, 'r', bfdout);
                if(flag==1){
                    printf("Row conflict\n");
                    continue;
                }
                flag=request_check(cn1fdout, c%3, d, 'c', bfdout);
                if(flag==1){
                    printf("Column conflict\n");
                    continue;
                }
                flag=request_check(cn2fdout, c/3, d, 'c', bfdout);
                if(flag==1){
                    printf("Column conflict\n");
                    continue;
                }
                B[c/3][c%3]=d;
                system("clear");
                print_board(B);
            }
        }
        else if(menu=='r' || menu=='c'){
            int i, d, fd;
            scanf("%d %d %d", &i, &d, &fd);
            reply_check(menu, i, d, fd, B);
        }
        else if(menu=='q'){
            system("clear");
            print_board(B);
            printf("Bye\n");
            sleep(2);
            exit(0);
        }
        sleep(2);
    }

}