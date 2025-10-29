#include <stdio.h>
//简单选择排序
void selectionSort(int arr[], int n) {
    for (int i = 0; i < n-1; i++) {
        int min_idx = i;
        for (int j = i+1; j < n; j++) {
            if (arr[j] < arr[min_idx]) {
                min_idx = j;
            }
        }
        // 交换
        int temp = arr[i];
        arr[i] = arr[min_idx];
        arr[min_idx] = temp;
    }
}


int main()
{
    int num[11];
    for (int i = 0 ; i < 10; i++){
        scanf("%d", &num[i]);
        getchar();
    }
    selectionSort(num, 10);
    for (int i = 0 ; i < 10; i++){
        printf("%d ", num[i]);
    }

    return 0;
}