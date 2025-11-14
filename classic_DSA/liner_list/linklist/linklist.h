#include <stdio.h>
#include <stdlib.h>

typedef int data_t;

typedef struct Node {
    data_t data;
    struct Node* next;
} listnode, *linklist;

linklist list_create();
int list_tail_insert(linklist H, data_t val);