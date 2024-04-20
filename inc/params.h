#pragma once
#include"sock.h"
#include <pthread.h>
#include"Window.h"
struct severTransParameter{
    int fd;
    SOCK sock;
    struct sockaddr_in clientSocketMsg;
    unsigned int clientMsgLen;
};

struct clientRecvParameter{
    int clientSocket;
    struct sockaddr_in severSocketMsg;
    unsigned int severMsgLen;
    WINDOW window;
    unsigned long blokeNum;
};

struct clientWriteParameter{
    int clientSocket;
    const char* path;
    WINDOW window;
    unsigned long blokeNum;
};