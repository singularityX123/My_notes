#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define N 64

typedef int data_t;

typedef struct {
    data_t data[N];
    int front, rear;
} sequeue_t;


