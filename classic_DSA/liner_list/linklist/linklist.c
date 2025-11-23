#include <stdio.h>
#include <stdlib.h>
#include "linklist.h"

linklist list_create(){
    //malloc
    linklist H = (linklist)malloc(sizeof(listnode)); //头结点

    //先判断是否为空
    if (H == NULL){
        printf("malloc failed\n");
        return NULL;
    }

    //initialize
    H->data = 0; //头结点的数据域置为0,表示链表长度为0
    H->next = NULL; //头结点的next指针置为空,断开了后续连接
    
    return H;
};

int list_tail_insert(linklist H, data_t val){
    if (H == NULL){
        printf("H is NULL\n");
        return -1;
    }

    //tail insert
    listnode* new_node = (listnode*)malloc(sizeof(listnode));
    if (new_node == NULL){
        printf("malloc failed\n");
        return -1;
    }
    new_node->data = val;
    new_node->next = NULL;

    listnode* p = H;
    while (p->next != NULL){
        p = p->next;
    }
    p->next = new_node;

    //update length
    H->data += 1;

    return 0;
}

int list_show(linklist H){
    if (H == NULL){
        printf("H is NULL\n");
        return -1;
    }

    listnode* p = H->next;
    while (p != NULL){
        printf("%d ", p->data);
        p = p->next;
    }
    printf("\n");

    return 0;
}