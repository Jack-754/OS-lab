#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]){
    FILE *file = fopen("foodep.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    int n, u;
    u=atoi(argv[1]);
    fscanf(file, "%d", &n);
    //printf("\n**%d\n", argc);
    if(argc==2){
        FILE *done = fopen("done.txt", "w");
        for(int i=0; i<n; i++){
            fprintf(done, "%d", 0);
        }
        fclose(done);
    }

    char line[10000];
    int tmp;
    char ctmp;
    int arr[10000];
    int size=0;

    fscanf(file, "%c", &ctmp);
    for(int i=0; i<n; i++){
        fscanf(file, "%d %c", &tmp, &ctmp);
        if(tmp==u){
            while (fscanf(file, "%d", &arr[size]) == 1) {
                size++;
            }
            break;
        }
        else{
            do{
                fscanf(file, "%c", &ctmp);
            }while(ctmp != '\n' && !feof(file));
        }
    }
    fclose(file);
    int status;
    if(u<n)size--;
    for(int i=0; i<size; i++){
        FILE *done=fopen("done.txt", "r");
        fscanf(file, "%s", line);
        fclose(done);
        if(line[arr[i]-1]=='1')continue;
        int pid=fork();
        if(pid==0){
            char *arglist[4];
            char str[10];
            snprintf(str, sizeof(str), "%d", arr[i]);
            arglist[0] = (char *)malloc(10*sizeof(char)); strcpy(arglist[0], argv[0]);
	        arglist[1] = str;
	        arglist[2] = str;
            arglist[3] = NULL;
            execvp(argv[0], arglist);
            printf("error\n");
        }
        else{
            wait(&status);
        }
    }
    
    FILE *done=fopen("done.txt", "r");
    fscanf(file, "%s", line);
    fclose(done);

    line[u-1]='1';

    done=fopen("done.txt", "w");
    fprintf(file, "%s", line);
    fclose(done);

    printf("foo%d rebuilt ", u);
    if(size!=0){
        printf("from ");
    }
    for(int i=0; i<size-1; i++){
        printf("foo%d, ", arr[i]);
    }
    if(size!=0){
        printf("foo%d", arr[size-1]);
    }
    printf("\n");

    exit(0);
    
}