#pragma once
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include<pthread.h>

typedef struct Sock{
    int socket;
    unsigned int port;
}*SOCK;

SOCK makeSocket(int port){
    SOCK sock=(SOCK)malloc(sizeof(struct Sock));
    memset(sock, 0, sizeof(struct Sock));

    sock->socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    struct sockaddr_in severSocketMsg;
    memset(&severSocketMsg, 0, sizeof(severSocketMsg));
    severSocketMsg.sin_family=AF_INET;
    severSocketMsg.sin_port=htons(port);
    severSocketMsg.sin_addr.s_addr=inet_addr("127.0.0.1");
    int severMsgLen=sizeof(severSocketMsg);
    //绑定ip和端口
    if (bind(sock->socket,(struct sockaddr *)&severSocketMsg,severMsgLen)==-1)
    {
        perror("SEVERSOCKET_BINDERROR\n");
        return NULL;
    }
    sock->port=ntohs(severSocketMsg.sin_port);
    return sock;
}