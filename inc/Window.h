#pragma once
#include "def.h"
#include"pack.h"
#include<stdio.h>
#include <stdlib.h>
#include <stdbool.h>
typedef struct Node{
    struct Pack pack;
    struct Node* nextNode; 
    struct Node* preNode;
}*NODE;

NODE createNode(struct Pack pack){
    NODE node=(NODE)malloc(sizeof(struct Node));
    memset(node, 0, sizeof(struct Node));
    node->pack=pack;
    node->nextNode=NULL;
    node->preNode=NULL;
    return node;
}

typedef struct Window{
    NODE head;
    NODE tail;
    int windowLen;
}*WINDOW;

WINDOW windowInit(){
    WINDOW window=(WINDOW)malloc(sizeof(struct Window));
    memset(window, 0, sizeof(struct Window));
    window->head=NULL;
    window->tail=NULL;
    window->windowLen=0;
    return window;
}

bool addToWindow(WINDOW window,struct Pack pack){

    if (window->windowLen==MAX_WINDOW_SIZE) {
        return false;
    }

    NODE node=createNode(pack);

    if (window->windowLen==0) {
        window->head=node;
        window->tail=node;
        window->windowLen++;
    }else {
        NODE temp=window->head;
        while (1) {
            if (temp==NULL) {
                window->tail->nextNode=node;
                node->preNode=window->tail;
                window->tail=node;
                window->windowLen++;
                return true;
            }else if(temp->pack.seq<node->pack.seq){
                temp=temp->nextNode;
                continue;
            }else {
                if (temp==window->head) {
                    node->nextNode=temp;
                    temp->preNode=node;
                    window->head=node;
                    window->windowLen++;
                    return true;
                }else{
                    node->preNode=temp->preNode;
                    node->nextNode=temp;
                    temp->preNode->nextNode=node;
                    temp->preNode=node;
                    window->windowLen++;
                    return true;
                }
            }
        }
    }
    //undefined behavior
    return false;
}

NODE popFromWindow(WINDOW window){
    if (window->windowLen==0) {
        perror("WINDOW OVERDLOW");
        return NULL;
    }

    NODE tempNode=window->head;
    if (window->windowLen==1) {
        window->head=NULL;
        window->tail=NULL;
    }else {
        window->head=window->head->nextNode;
    }
    window->windowLen--;
    //需要手动释放返回节点
    return tempNode;
}

//暂时没用到
void elemShift(WINDOW window){
    if (window->windowLen==1) {
        perror("SHIFT ERROR");
        return;
    }else{
        NODE temp=window->head;
        window->head=window->head->nextNode;
        temp->nextNode=NULL;
        window->tail->nextNode=temp;
        window->tail=window->tail->nextNode;
    }
}