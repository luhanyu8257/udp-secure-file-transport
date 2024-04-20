#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include"pack.h"
#include "def.h"
int getDir(char* path,char*pathAddrs[MAX_PATH_NUM]){

    DIR* dir=opendir(path);
    if (dir==NULL) {
        perror("DIR OPEN ERROR");

    }
    struct dirent* dirBar;
    int pathNum=0;
    while ((dirBar=readdir(dir))!=NULL) {

        
        if (strcmp(dirBar->d_name, ".")==0) {
            continue;
        }else if(strcmp(dirBar->d_name, "..")==0){
            continue;
        }else {
            int len=strlen(dirBar->d_name);
            char* str=(char*)malloc(sizeof(char)*(len+3));
            memset(str, 0, sizeof(char)*(len+3));
            if (dirBar->d_type==DT_DIR) {
                str[0]='d';
                str[1]='_';
                str[2]=' ';
                pathAddrs[pathNum++]=strcat(str, dirBar->d_name);
            }else {
                str[0]='f';
                str[1]='_';
                str[2]=' ';
                pathAddrs[pathNum++]=strcat(str, dirBar->d_name);
            }
        }
    }

    closedir(dir);
    return pathNum;
}

int pathMerge(char**pathAddrs,int pathNum,PACK pSever){
    
    int packLen=0;
    for (int i=0;i<pathNum ; i++) {
        strcat(pSever->data, pathAddrs[i]);
        strcat(pSever->data, "\n");
        packLen=packLen+strlen(pathAddrs[i])+1;
        free(pathAddrs[i]);
    }
    
    return packLen;
}
