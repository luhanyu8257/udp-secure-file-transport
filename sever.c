#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include "inc/FileOpra.h"
#include "inc/def.h"
#include "inc/pack.h"
#include <time.h>
#include <wait.h>
#include <sys/file.h>
#include <pthread.h>
#include<sys/select.h>
#include"inc/multiThreads.h"
#include"inc/params.h"

int main(int argc,char* argv[]){
    pthread_mutex_init(&severMutex, NULL);
    //创建服务器socket
    int severSocket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    struct sockaddr_in severSocketMsg;
    memset(&severSocketMsg, 0, sizeof(severSocketMsg));
    severSocketMsg.sin_family=AF_INET;
    int basePort=8000;
    severSocketMsg.sin_port=htons(basePort);
    severSocketMsg.sin_addr.s_addr=inet_addr("127.0.0.1");
    int severMsgLen=sizeof(severSocketMsg);
    //绑定ip和端口
    if (bind(severSocket,(struct sockaddr *)&severSocketMsg,severMsgLen)==-1)
    {
        perror("SEVERSOCKET_BINDERROR\n");
        return -1;
    }

    //创建接收方信息
    struct sockaddr_in clientSocketMsg;
    unsigned int clientMsgLen=sizeof(clientSocketMsg);

    //建立服务器缓冲区
    PACK pSever=createPack();

    //根目录
    char* path=(char*)malloc(sizeof(char)*MAXPATHLEN);
    memset(path, 0 ,sizeof(char)*MAXPATHLEN);
    strcat(path, "//home//wowotou//桌面//vscode//test");

    //收消息
    while (1) {
        //重置缓冲区
        memset(pSever,0,sizeof(char)*sizeof(struct Pack));  
        
        //接收客户端命令
        int dataLen=recvfrom(severSocket,pSever,sizeof(struct Pack),0,(struct sockaddr*)&clientSocketMsg,&clientMsgLen);

        //拷贝客户端命令
        char* temp=malloc(sizeof(char)*sizeof(struct Pack));
        strcpy(temp, pSever->data);

        //分割客户端命令
        char* seq_1=strtok(temp, " ");
        char* seq_2=strtok(NULL, " ");
        char* seq_3=strtok(NULL, " ");
        
        //命令解析
        if (strcmp(pSever->data,"get col")==0){

            printf("进入get col\n");
            //目录获取命令
            makeColumPack(pSever, path);
            sendto(severSocket,pSever,sizeof(struct Pack) ,0,(struct sockaddr*)&clientSocketMsg,clientMsgLen);

        }else if(strcmp(seq_1, "cd")==0) {

            printf("进入cd\n");
            //地址有效性检验
            bool ret=pathExist(path, seq_2);

            if (!ret) {
                makeErrorPack(pSever, "PATH NOT EXIST\n");
                sendto(severSocket,pSever,sizeof(struct Pack),0,(struct sockaddr*)&clientSocketMsg,clientMsgLen);
            }else {
                //修改目录
                strcat(path, "//");
                strcat(path, seq_2);   
                makeColumPack(pSever, path);
                sendto(severSocket,pSever,sizeof(struct Pack),0,(struct sockaddr*)&clientSocketMsg,clientMsgLen);

            }
           
        }else if((strcmp(seq_1, "get")==0)&&(seq_3!=NULL)){

            printf("进入get file\n");
            int ret=pathExist(path, seq_2);

            //地址有效性检验
            if (!ret) {
                memset(pSever, 0, sizeof(struct Pack));
                makeErrorPack(pSever, "FILE NOT EXIST\n");
                sendto(severSocket,pSever,sizeof(struct Pack),0,(struct sockaddr*)&clientSocketMsg,clientMsgLen);
            }else {
                //文件传输

                //获得临时文件地址
                char* tempPath=(char*)malloc(MAXPATHLEN);
                memset(tempPath, 0, MAXPATHLEN);
                strcpy(tempPath, path);
                strcat(tempPath, "//");
                strcat(tempPath, seq_2);
                makeMsgPack(pSever, tempPath);
                sendto(severSocket,pSever,sizeof(struct Pack),0,(struct sockaddr*)&clientSocketMsg,clientMsgLen);
                

                //构造发送线程参数
                ssize_t fd=open(tempPath,O_RDONLY);
                
                SOCK socks[MAXTHREADNUM];
                struct severTransParameter param[MAXTHREADNUM];
                srand(time(NULL));
                unsigned int portBegin=1025+rand()%(65535-1025-MAXTHREADNUM);
                for (int i=0; i<MAXTHREADNUM; i++) {
                    printf("port :%d\n",portBegin);
                    socks[i]=makeSocket(portBegin%65535);
                    if (socks[i]==NULL) {
                        printf("error port %d\n",portBegin);
                        i--;
                        portBegin++;
                        continue;
                    }
                    param[i].fd=fd;
                    param[i].clientSocketMsg=clientSocketMsg;
                    param[i].clientMsgLen=clientMsgLen;
                    param[i].sock=socks[i];
                    portBegin++;
                }

                //发送线程创建，初始化锁
                
                pthread_t tids[MAXTHREADNUM];
                for (int i=0; i<MAXTHREADNUM; i++) {
                    pthread_create(&tids[i], NULL, serverTransThread, (void*)&param[i]);
                }
                
                for (int i=0; i<MAXTHREADNUM; i++) {
                    pthread_join(tids[i], NULL);
                    close(socks[i]->socket);
                }         
            }
        }else {
            printf("ERROR COMMAND\n");
            makeErrorPack(pSever, "ERROR COMMAND\n");
            sendto(severSocket,pSever,sizeof(struct Pack),0,(struct sockaddr*)&clientSocketMsg,clientMsgLen);
        }
        printf("done\n");
    }
    pthread_mutex_destroy(&severMutex);
    close(severSocket);
    return 0;
}