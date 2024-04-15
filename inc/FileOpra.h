#pragma once
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include "ColumOpra.h"
#include "pack.h"
#include <wait.h>
#include<arpa/inet.h>
#include "Window.h"
#include<pthread.h>
unsigned char *generateMD5(char *data){
    // unsigned char* md5=(unsigned char*)malloc(sizeof(char)*(MD5_DIGEST_LENGTH+1));
    // memset(md5, 0, sizeof(char)*(MD5_DIGEST_LENGTH+1));

    // MD5_CTX m;
    // int ret=MD5_Init(&m);
    // if(ret!=1){
    //     perror("MD5 GENERATE ERROR\n");
    //     return NULL;
    // }
    // MD5_Update(&m, data, strlen(data));
    // MD5_Final(md5,&m);
    // unsigned char* hexMD5=(unsigned char *)malloc(sizeof(char)*(MD5_DIGEST_LENGTH*2+1));
    // memset(hexMD5, 0, sizeof(char)*(MD5_DIGEST_LENGTH*2+1));

    // for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
	// {
	// 	sprintf((char*)&hexMD5[i * 2], "%02X", md5[i]);         //查看MD5
	// }
    // return hexMD5;
    EVP_MD_CTX *mdctx;
    unsigned char *md5_digest;
    unsigned int md5_digest_len = EVP_MD_size(EVP_md5());
    // MD5_Init
    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);

    // MD5_Update
    EVP_DigestUpdate(mdctx, data, strlen(data));

    // MD5_Final
    md5_digest = (unsigned char *)malloc(md5_digest_len);
    memset(md5_digest, 0, md5_digest_len);

    EVP_DigestFinal_ex(mdctx, md5_digest, &md5_digest_len);
    EVP_MD_CTX_free(mdctx);
    
    unsigned char* hexMD5=(unsigned char *)malloc(sizeof(char)*(md5_digest_len*2+1));
    memset(hexMD5, 0, sizeof(char)*(md5_digest_len*2+1));
    for (int i = 0; i < md5_digest_len; i++){
		sprintf((char*)&hexMD5[i * 2], "%02X", md5_digest[i]);         //查看MD5
	}
    free(md5_digest);
    return hexMD5;
}

bool pathExist(char* path,char* next){
    //地址有效性检验
    char* filePath=(char*)malloc(sizeof(char)*MAX_PATH_LEN);
    memset(filePath, 0, sizeof(char)*MAX_PATH_LEN);
    strcpy(filePath, path);
    strcat(filePath, "//");
    strcat(filePath,next);  
    if (fopen(filePath, "r")==NULL) {
        perror("PATH NOT EXIST");
        free(filePath);
        return false;
    }else {
        free(filePath); 
        return true;
    }    
}

int makeAckPack(PACK pack,unsigned int seq){
    pack->type='0';
    pack->seq=seq ;
    return 0;
}

int makeFinPack(PACK pack){
    pack->type='1';
    return 0;
}

int makeCmdPack(PACK pack,char* cmd){
    pack->type='2';
    strcat(pack->data, cmd);
    return 0;
}

int makeColumPack(PACK pack,char* path){
    pack->type='3';
    //创建目录数组
    char*pathAddrs[MAX_PATH_NUM];
    memset(pathAddrs, 0,sizeof(pathAddrs));
    //构造目录数据包
    int pathNum=getDir(path,pathAddrs);        
    memset(pack,0, sizeof(struct Pack));
    int packLen=pathMerge(pathAddrs, pathNum, pack);    
    return 0;
}

int makeDataPack(PACK pack,char* buf,unsigned long seq,unsigned int len){
    //构建数据包
    pack->type='4';
    pack->seq=seq;
    pack->dataLen=len;
    memcpy(pack->data, buf, len);
    unsigned char* hexMD5;
    hexMD5=generateMD5(pack->data);
    memcpy(pack->md5,hexMD5,33);
    free(hexMD5);
    return len;
}

int makeErrorPack(PACK pack,const char* errorCode){
    //构造错误信息数据包
    pack->type='5';
    strcat(pack->data, errorCode);
    return 0;
}

int makeMsgPack(PACK pack,char* path){
    pack->type='6';
    //获取文件大小
    struct stat fileState;
    stat(path, &fileState);
    unsigned long fileSize=fileState.st_size;
    //构造文件大小数据包
    char* dataLen=(char*)malloc(20);
    memset(dataLen, 0, sizeof(char)*20);
    sprintf(dataLen, "%ld", fileSize);
    memset(pack, 0, sizeof(struct Pack));
    strcat(pack->data, dataLen);
    free(dataLen);
    return 0;
}

bool md5Check(PACK pack){
    //校验数据包的md5
    bool ret;
    unsigned char* temp=generateMD5(pack->data);
    if (!strcmp((char*)temp, (char*)pack->md5)) {
        ret= true;
    }else {
        ret= false;
    }
    free(temp);
    return ret;
}