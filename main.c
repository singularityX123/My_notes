#include <stdio.h>

void func(int **p) {
    int x = 100;
    *p = &x;
}

int main() {
    char str1[] = "Hello";
    char str2[] = "Hello";
    if (str1 == str2) {
        printf("Equal\n");
    } else {
        printf("Not Equal\n");
    }
    return 0;
}