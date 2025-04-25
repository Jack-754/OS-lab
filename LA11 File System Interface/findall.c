#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>


#define MAX_USERS 1024 // Adjust as needed
#define MAX_NAME_LEN 50

typedef struct uidToLoginid {
    int uid;
    char loginid[MAX_NAME_LEN + 1]; // +1 for null terminator
} uidToLoginid;

uidToLoginid uid_map[MAX_USERS];
int uid_count = 0;

void build_uid_map(const char *passwd_path) {
    FILE *fp = fopen(passwd_path, "r");
    if (!fp) {
        perror("Failed to open passwd file");
        exit(1);
    }
    
    char line[1000];
    while (fgets(line, sizeof(line), fp)) {
        char *username = strtok(line, ":");
        if (!username) continue;
        strtok(NULL, ":"); 
        char *uid_str = strtok(NULL, ":");
        if (!uid_str) continue;
        int uid = atoi(uid_str);
        
        if (uid_count < MAX_USERS) {
            uid_map[uid_count].uid = uid;
            strncpy(uid_map[uid_count].loginid, username, MAX_NAME_LEN);
            uid_map[uid_count].loginid[MAX_NAME_LEN] = '\0'; 
            uid_count++;
        }
    }
    
    fclose(fp);
}

const char* uid_to_login(int uid) {
    for (int i = 0; i < uid_count; i++) {
        if (uid_map[i].uid == uid)
            return uid_map[i].loginid;
    }
    return "Unknown"; // Not found
}

int count=1;

int cmpext(char *filename, char *extension){
    int len1 = strlen(filename);
    int len2 = strlen(extension);
    if(len1<=len2)return 0;
    if(filename[len1-len2-1]!='.')return 0;
    return strcmp(filename+len1-len2, extension)==0;
}

void printfileinfo(char *filename){
    struct stat fileStat;
    if(stat(filename, &fileStat)<0){
        perror("stat");
        return;
    }
    printf("%d\t\t:%s\t\t%ld\t\t%s\n", count++, uid_to_login(fileStat.st_uid), fileStat.st_size, filename);
}

void findfile(char *directory, char *extension){
    DIR *dir=opendir(directory);
    struct dirent *entry;
    int idx=1;
    if(dir!=NULL){
        while(entry=readdir(dir)){
            if(entry->d_type==DT_DIR){
                if(strcmp(entry->d_name, ".")!=0 && strcmp(entry->d_name, "..")!=0){
                    char newdir[1000];
                    snprintf(newdir, sizeof(newdir), "%s/%s", directory, entry->d_name);
                    findfile(newdir, extension);
                }
            }
            else if(entry->d_type==DT_REG){
                if(cmpext(entry->d_name, extension)){
                    char filepath[1000];
                    snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);
                    printfileinfo(filepath);
                }
            }
        }
        closedir(dir);
    }
    else{
        printf("Couldn't open directory %s\n", directory);
    }
}

int main(int argc, char **argv){

    if(argc<3){
        printf("Run with command line arguments: <directory> <extension>\n");
        return 1;
    }

    char *directory = argv[1];
    char *extension = argv[2];

    build_uid_map("/etc/passwd");

    printf("NO\t\t:OWNER\t\tSIZE\t\tNAME\n");
    printf("--\t\t -----\t\t----\t\t----\n");
    findfile(directory, extension);
    
    printf("+++ %d files match the extension %s", count-1, extension);
    
    return 0;
}