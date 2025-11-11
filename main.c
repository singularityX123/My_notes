#include <stdio.h>

struct Node { int data; struct Node* next; };
int main() {
    struct Node a = {10}, b = {20};
    a.next = &b;
    printf("%d\n", a.next->data);
    return 0;
}
