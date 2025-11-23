#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlist.h"

sqlink list_create(){
    //malloc
    sqlink L = (sqlink)malloc(sizeof(sqlist));

    //先判断是否为空
    if (L == NULL){
        printf("malloc failed\n");
        return NULL;
    }

    //initialize
    memset(L,0,sizeof(sqlist));
    L->last = -1; //数组从0开始，所以-1表示空表
    
    return L;
};

int list_clear(sqlink L){
    if (L == NULL){
        printf("L is NULL\n");
        return -1;
    }

    //clear
    memset(L,0,sizeof(sqlist)); // 清空结构体,粉碎内存信息
    L->last = -1; 
    return 0;
};

int list_empty(sqlink L){
    if (L == NULL){
        printf("L is NULL\n");
        return -1;
    }

    //Is list empty?
    if (L->last == -1){
        return 0;
    }
    return 1;
};

int list_length(sqlink L){
    if (L == NULL){
        printf("L is NULL\n");
        return -1;
    }
    return L->last + 1;
};

int list_locate(sqlink L, data_t val){
    if (L == NULL){
        printf("L is NULL\n");
        return -1;
    }
    
    if (L->last == -1){
        printf("list is empty\n");
        return -1;
    }

    //Locate
    for (int i = 0; i <= L->last; i++){
        if (L->data[i] == val){
            return i;
        }
    }
    return -1;
};

int list_locate_all(sqlink L, data_t val, int positions[], int max_positions) {
    if (L == NULL) {
        printf("L is NULL\n");
        return -1;
    }
    
    if (L->last == -1) {
        printf("list is empty\n");
        return -1;
    }
    
    if (positions == NULL || max_positions <= 0) {
        printf("invalid positions array\n");
        return -1;
    }
    
    int count = 0;
    for (int i = 0; i <= L->last && count < max_positions; i++) {
        if (L->data[i] == val) {
            positions[count] = i;
            count++;
        }
    }
    
    return count;
}
int list_insert(sqlink L, data_t val, int pos){
    if (L == NULL){
        printf("L is NULL\n");
        return -1;
    }

    //Is pos valid?
    if (pos < 0 || pos > L->last + 1){
        printf("pos is invalid\n");
        return -1;
    }

    //Is list full?
    if (L->last == N-1){
        printf("list is full\n");
        return -1;
    }

    //Move
    for (int i = L->last; i >= pos; i--){
        L->data[i+1] = L->data[i];
    }

    //Insert
    L->data[pos] = val;
    L->last++;
    return 0;
}

int list_delete(sqlink L, int pos){
    if (L == NULL){
        printf("L is NULL\n");
        return -1;
    }

    //Is pos valid?
    if (pos < 0 || pos > L->last){
        printf("pos is invalid\n");
        return -1;
    }

    //Move2delete
    for (int i = pos; i < L->last; i++){
        L->data[i] = L->data[i+1];
    }
    
    L->last--;

    return 0;
}

int list_purge(sqlink L){
    if (L == NULL){
        printf("L is NULL\n");
        return -1;
    }

    //fast_purge
    // L->last = -1;

    free(L);
    L = NULL;
    
    return 0;
}

int list_reverse(sqlink L){
    if (L == NULL){
        printf("L is NULL\n");
        return -1;
    }

    if (L->last == -1){
        printf("list is empty\n");
        return -1;
    }

    //Reverse
    for (int i = 0; i <= L->last / 2; i++){
        data_t temp = L->data[i];
        L->data[i] = L->data[L->last - i];
        L->data[L->last - i] = temp;
    }

    return 0;
}

// TODO:根据业务需求任意合并顺序表
int list_merge(sqlink L1, sqlink L2){
    if (L1 == NULL || L2 == NULL){
        printf("L1 or L2 is NULL\n");
        return -1;
    }

    for (int i = 0; i <= L2->last; i++){
        list_insert(L1, L2->data[i], L1->last + 1);
    }

    return 0;
}

int list_clean_repeat(sqlink L){
    if (L == NULL){
        printf("L is NULL\n");
        return -1;
    }

    if ((L->last == -1) || (L->last == 0)){
        printf("There is no need to clean\n");
        return -1;
    }

    //Clean
    for (int i = 0; i <= L->last; i++){
        for (int j = i + 1; j <= L->last; j++){
            if (L->data[i] == L->data[j]){
                list_delete(L, j);
                j--; // 删除后的后续索引调整
            }
        }
    }

    return 0;
}