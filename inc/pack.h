#pragma once
#include "def.h"
#include <string.h>
#include <stdlib.h>

typedef struct Pack{
    char type; //1BYTE 0:ack 1:fin 2:cmd 3:colum 4:data 5.error 6.msg
    unsigned long seq;  //8BYTE包序号
    unsigned int dataLen;//4BYTE包中数据长度
    unsigned char md5[33];  //32BYTE
    char data[MAXPACKDATALEN]; //1024BYTE
    int port;  //发送线程端的端口号
}*PACK;

PACK createPack(){
    PACK p=(PACK)malloc(sizeof(struct Pack));
    memset(p, 0, sizeof(struct Pack));
    return p;
}

