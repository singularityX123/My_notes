#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define N 64

typedef int data_t;

typedef struct {
    data_t data[N];
    int front, rear;
} sequeue_t;


//创建队列 Queue_create()

//清空队列 Queue_clear(L)

//判断队列是否为空 Queue_is_empty(L)

//判断队列是否已满 Queue_is_full(L) 为区别，满队元素个数比队列长度少一个（循环队列）

//入队 Queue_enqueue(L, x)

//出队 Queue_dequeue(L)
