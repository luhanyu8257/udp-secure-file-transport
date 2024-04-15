#pragma once
#include "def.h"
#include"pack.h"
#include"params.h"
#include "FileOpra.h"
#include "Window.h"
#include "sock.h"
#include <netinet/in.h>
#include <pthread.h>
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
pthread_mutex_t severMutex;
pthread_mutex_t clientMutex;

void* serverTransThread(void* param){
    //发送线程
    struct severTransParameter* para=(struct severTransParameter*)param;
    char buf[MAX_PACK_DATA_LEN];
    struct Pack sendPacket;
    struct Pack recvPacket;
    unsigned long seq;
    int len;
    while (1) {
        memset(buf, 0, MAX_PACK_DATA_LEN);
        pthread_mutex_lock(&severMutex);
        len=read(para->fd,buf,MAX_PACK_DATA_LEN);
        unsigned long end=lseek(para->fd, 0, SEEK_CUR);
        seq=end/MAX_PACK_DATA_LEN;
        if (end%MAX_PACK_DATA_LEN!=0) {
            seq++;
        }
        pthread_mutex_unlock(&severMutex);
        if (len==0) {
            return NULL;
        }
        makeDataPack(&sendPacket, buf, seq, len);
        //记录端口
        sendPacket.port=para->sock->port;   
        do {
            sendto(para->sock->socket,&sendPacket,sizeof(struct Pack),0,(struct sockaddr*)&para->clientSocketMsg,para->clientMsgLen);
            recvfrom(para->sock->socket,&recvPacket,sizeof(struct Pack),0,(struct sockaddr*)&para->clientSocketMsg,&para->clientMsgLen);
            printf("port:%dseq:%lureqseq:%ld\n",para->sock->port,seq,recvPacket.seq);
            if (recvPacket.seq!=seq) {
                usleep(10);
            }
        }while (recvPacket.seq!=seq);
    }
    return 0;
}

void* clientRecvThread(void * param){
    struct clientRecvParameter* para=(struct clientRecvParameter*)param;
    //接受数据包
    PACK requestPack=(PACK)malloc(sizeof(struct Pack));
    PACK dataPack=(PACK)malloc(sizeof(struct Pack));

    unsigned long num=1;
    while (num<=para->blokeNum) {
        
        memset(requestPack, 0, sizeof(struct Pack));   
        memset(dataPack, 0, sizeof(struct Pack)); 
        
        recvfrom(para->clientSocket, dataPack,sizeof(struct Pack),0,(struct sockaddr*)&para->severSocketMsg,&para->severMsgLen);
        
        pthread_mutex_lock(&clientMutex);

        if (para->window->windowLen==MAX_WINDOW_SIZE) {
            makeAckPack(requestPack, num);
            sendto(para->clientSocket, requestPack, sizeof(struct Pack), 0, (struct sockaddr*)&para->severSocketMsg, para->severMsgLen);
            //队列满，线程挂起，等待队列清空
            pthread_mutex_unlock(&clientMutex);
            usleep(UHUNG_TIME); 

        }else if (!md5Check(dataPack)) {
            pthread_mutex_unlock(&clientMutex);
            makeAckPack(requestPack, num);
            sendto(para->clientSocket, requestPack, sizeof(struct Pack), 0, (struct sockaddr*)&para->severSocketMsg, para->severMsgLen);
            
        }else {
            addToWindow(para->window, *dataPack);
            printf("%lu\n",para->window->head->pack.seq);
            pthread_mutex_unlock(&clientMutex);  
            num++;

            //队列满，线程挂起，等待队列清空
            if (para->window->windowLen==MAX_WINDOW_SIZE) {
                usleep(UHUNG_TIME); 
                continue;
            }
        }
    }
    free(requestPack);
    free(dataPack);    
    //undefined behavior
    return NULL;
}

void* clientWriteThread(void* param){
    //创建发送端口信息
    struct sockaddr_in severSocketMsg;
    memset(&severSocketMsg, 0, sizeof(severSocketMsg));
    severSocketMsg.sin_family=AF_INET;
    severSocketMsg.sin_addr.s_addr=inet_addr(server_addr);
    unsigned int severMsgLen=sizeof(severSocketMsg);
    
    struct clientWriteParameter* para =(struct clientWriteParameter*)param;
    ssize_t fd=open(para->path, O_WRONLY|O_APPEND|O_TRUNC|O_CREAT,0644);
    if (fd==-1) {
        perror("CLIENT FILE OPEN ERROR");
    }

    int num=1;
    PACK requestPack=(PACK)malloc(sizeof(struct Pack));
    while (num<=para->blokeNum) {
        memset(requestPack, 0, sizeof(struct Pack));
        pthread_mutex_lock(&clientMutex);
        if (para->window->windowLen!=0) {
            if (para->window->head->pack.seq==num) {

                NODE node=popFromWindow(para->window);
                int len =para->window->windowLen;
                pthread_mutex_unlock(&clientMutex);

                write(fd, node->pack.data, node->pack.dataLen);
                makeAckPack(requestPack, node->pack.seq);
                severSocketMsg.sin_port=htons(node->pack.port);
                sendto(para->clientSocket, requestPack, sizeof(struct Pack), 0, (struct sockaddr*)&severSocketMsg, severMsgLen);
                printf("num:%d\n",num);
                printf("write%lu  windowlen:%d\n",node->pack.seq,len);
                free(node);
                num++;
            }else{
                pthread_mutex_unlock(&clientMutex);
                usleep(UHUNG_TIME);
            }
        }else {
            
            pthread_mutex_unlock(&clientMutex);
            usleep(UHUNG_TIME);
        }
    }
    close(fd);
    //undefined behavior
    return NULL;
}
