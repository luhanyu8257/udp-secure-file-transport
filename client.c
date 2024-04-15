#include <pthread.h>
#include<stdio.h>
#include <stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include <netinet/in.h>
#include<arpa/inet.h> 
#include<string.h>
#include "inc/FileOpra.h"
#include "inc/def.h"
#include"inc/multiThreads.h"
#include "inc/params.h"
#include "inc/Window.h"
int main(int argc,char* argv[]){
    if(argc>2){
        char *server_addr=argv[1];
        server_port=atoi(argv[2]);
    }

    //初始化锁
    pthread_mutex_init(&clientMutex, NULL);
    //创建客户端socket
    int clientSocket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    struct sockaddr_in clientSocketMsg;
    memset(&clientSocketMsg, 0, sizeof(clientSocketMsg));
    clientSocketMsg.sin_family=AF_INET;
    clientSocketMsg.sin_port=htons(9000);
    clientSocketMsg.sin_addr.s_addr=inet_addr("127.0.0.1");
    unsigned int clientMsgLen=sizeof(clientSocketMsg);
    //绑定ip和端口
    if (bind(clientSocket,(struct sockaddr *)&clientSocketMsg,clientMsgLen)==-1)
    {
        perror("CLIENTSOCKET_BINDERROR\n");
        return -1;
    }


    //创建服务器地址和端口
    struct sockaddr_in severSocketMsg;
    memset(&severSocketMsg, 0, sizeof(severSocketMsg));
    severSocketMsg.sin_family=AF_INET;
    severSocketMsg.sin_port=htons(server_port);
    severSocketMsg.sin_addr.s_addr=inet_addr(server_addr);
    unsigned int severMsgLen=sizeof(severSocketMsg);

    //客户端缓冲区
    PACK pClient=createPack();

    
    //接收目录
    char* path=(char*)malloc(sizeof(char)*MAX_PATH_NUM);
    memset(path, 0 ,sizeof(char)*MAX_PATH_NUM);
    strcat(path, "//home//wowotou//桌面//vscode//test");


    //用于分割客户端命令
    char* seq_1;
    char* seq_2;
    char* dest_path;

    //用于拷贝客户端命令
    char* temp=malloc(MAX_INSTRUCT_LEN);
    
    //输入命令
    while (1) {

        //重置缓冲区
        memset(pClient,0,sizeof(struct Pack));

        //输入指令，并清除输入末尾的\n
        char* command=(char*)malloc(MAX_INSTRUCT_LEN);
        memset(command, 0, MAX_INSTRUCT_LEN);

        while (1) {
            fgets(command,MAX_INSTRUCT_LEN,stdin);
            command[strlen(command)-1]='\0';

            //拷贝客户端命令
            strcpy(temp, command);

            //指令分割
            seq_1=strtok(temp, " ");
            seq_2=strtok(NULL, " ");
            dest_path=strtok(NULL, " ");

            if (strlen(command)==1) {
                continue;
            }else if (seq_2==NULL) {
                printf("ERROR COMMAND\n");
                continue;
            }else if (strcmp(seq_1, "get")==0&&dest_path==NULL&&strcmp(seq_2, "col")!=0) {
                printf("ERROR COMMAND\n");
                continue;
            }else{ 
                break;
            }
        }
        
        //创建命令包
        makeCmdPack(pClient, command);

        //发送命令
        sendto(clientSocket,pClient,sizeof(struct Pack),0,(struct sockaddr*)&severSocketMsg,severMsgLen);
        
        //解析命令
        if (strcmp(command, "get col")==0) {

            printf("客户端请求get col\n");
            memset(pClient,0,sizeof(struct Pack));
            int dataLen=recvfrom(clientSocket,pClient,sizeof(struct Pack),0,(struct sockaddr*)&severSocketMsg,&severMsgLen);
            if (dataLen>0)
            {
                printf("%s\n",pClient->data);
            }

        }else if (strcmp(seq_1,"cd")==0) {

            printf("客户端请求cd\n");
            memset(pClient,0, sizeof(struct Pack));
            int dataLen=recvfrom(clientSocket,pClient,sizeof(struct Pack),0,(struct sockaddr*)&severSocketMsg,&severMsgLen);
            if (dataLen>0)
            {
                printf("error:%s\n",pClient->data);
            }
        }else{
            printf("客户端请求get file somewhere\n");
            //创建临时地址
            char* tempPath=(char*)malloc(MAX_PATH_LEN);
            memset(tempPath, 0, MAX_PATH_LEN);
            strcpy(tempPath, path);
            strcat(tempPath, "//");
            strcat(tempPath, dest_path);

            recvfrom(clientSocket,pClient,sizeof(struct Pack),0,(struct sockaddr*)&severSocketMsg,&severMsgLen);
            //检验地址有效性
            if (pClient->type=='5') {
                printf("%s",pClient->data);
                printf("done\n");
                continue;
            }

            //接收文件包数
            unsigned long fileSize=atol(pClient->data);
            unsigned long blokeNum=fileSize/MAX_PACK_DATA_LEN+1;
            
            //接收队列
            WINDOW window=windowInit();
            pthread_t tids[2];
            memset(&tids, 0, sizeof(pthread_t)*2);
            //构造clientRecvParameter包
            struct clientRecvParameter recvparam;
            recvparam.window=window;
            recvparam.blokeNum=blokeNum;
            recvparam.severMsgLen=severMsgLen;
            recvparam.severSocketMsg=severSocketMsg;
            recvparam.clientSocket=clientSocket;
            
            //clientRecvThread发射
            pthread_create(&tids[0], NULL, clientRecvThread, (void*)&recvparam);

            //构建clientWriteParameter包
            struct clientWriteParameter writeParam;
            writeParam.blokeNum=blokeNum;
            writeParam.path=tempPath;
            writeParam.window=window;
            writeParam.clientSocket=clientSocket;
            //clientWriteThread发射
            pthread_create(&tids[1], NULL, clientWriteThread, (void*)&writeParam);
            for (int i=0; i<2; i++) {
                pthread_join(tids[i], NULL);
            }
            printf("回收\n");
            
            free(tempPath);
        }

        printf("over\n");
    }
    //销毁锁
    pthread_mutex_destroy(&clientMutex);
    close(clientSocket);
    return 0;
}