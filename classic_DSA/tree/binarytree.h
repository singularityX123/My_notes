#include <stdio.h>
#include <stdlib.h>

typedef char data_t;

typedef struct node_t {
    data_t data;
    struct node_t *lchild;
    struct node_t *rchild;
} bit_tree;

bit_tree* create_bit_tree();

