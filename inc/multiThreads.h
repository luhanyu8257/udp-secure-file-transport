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
#include <sys/time.h>
pthread_mutex_t severMutex;
pthread_mutex_t clientMutex;
int myRecvFrom(int socket,PACK pack,int len,struct sockaddr* SocketMsg,socklen_t* MsgLen,unsigned long* delaytime,int num){
    struct timeval begin,end;
    gettimeofday(&begin, NULL);
    int dataLen=-1;
    int n=0;
    //如果轮询时间超过3倍的rtt，那么启动超时重发
    while (dataLen==-1 && n < 3) {
        printf("hhhhhhhh1\n");
        usleep(*delaytime);
        printf("hhhhhhhh2\n");
        dataLen=recvfrom(socket,pack, len,0,SocketMsg,MsgLen);
        if (dataLen==-1) {
            printf("1\n");
            //未收到返回包，继续等待
            n++;   
        }else if(pack->seq<num){
            printf("2\n");
            //收到超时重发的回复包，计时器清零，继续等待
            num=0;
            gettimeofday(&begin, NULL);
            continue;
        }else{
            printf("3\n");
            //对话结束，或收到当前等待的回复包，跳出循环
            break;
        }
    }
    gettimeofday(&end, NULL);
    *delaytime=(unsigned long)((end.tv_sec-begin.tv_sec)*1000*1000+end.tv_usec-begin.tv_usec)*0.97;    
    printf("delaytime:%lu\n",*delaytime);
    return dataLen;
}
void* serverTransThread(void* param){
    //发送线程
    struct severTransParameter* para=(struct severTransParameter*)param;
    char buf[MAX_PACK_DATA_LEN];
    struct Pack sendPacket;
    struct Pack recvPacket;
    unsigned long seq;
    int len;
    unsigned long udelaytime =DEFAULT_BLOKE_TIME;
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
            myRecvFrom(para->sock->socket,&recvPacket,sizeof(struct Pack),(struct sockaddr*)&para->clientSocketMsg,&para->clientMsgLen, &udelaytime, seq);
            //recvfrom(para->sock->socket,&recvPacket,sizeof(struct Pack),0,(struct sockaddr*)&para->clientSocketMsg,&para->clientMsgLen);
            printf("port:%dseq:%lureqseq:%ld\n",para->sock->port,seq,recvPacket.seq);
            // if (recvPacket.seq!=seq) {
            //     usleep(10);
            // }
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
