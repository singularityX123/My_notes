#include <stdio.h>
#include "linklist.h"

int main(){
    linklist H = list_create();
    int val;

    if (H == NULL){
        printf("list_create failed\n");
        return -1;
    }

    printf("please input numbers:\n");
    while (1){
        scanf("%d", &val);
        if (val == -1){
            break;
        }
        list_tail_insert(H, val);
        printf("please input numbers:\n");
    }

    list_show(H);
    
    return 0;
}