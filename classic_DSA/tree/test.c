#include <stdio.h>
#include <stdlib.h>

typedef struct TreeNode {
    int data;
    struct TreeNode* left;
    struct TreeNode* right;
} TreeNode;

TreeNode* createNode(int data) {
    TreeNode* newNode = (TreeNode*)malloc(sizeof(TreeNode));
    if (newNode == NULL) {
        exit(1);
    }
    newNode->data = data;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}


TreeNode* create_tree() {
    TreeNode* nodes[10];
    for (int i = 0; i < 10; i++) {
        nodes[i] = createNode(i + 1);
    }
    
    for (int i = 0; i < 10; i++) {
        int lchild = 2 * i + 1;
        int rchild = 2 * i + 2;
        if (lchild < 10) {
            nodes[i]->left = nodes[lchild];
        }
        if (rchild < 10) {
            nodes[i]->right = nodes[rchild];
        }
    }
    
    // 返回根节点
    return nodes[0];
}

int main() {

    TreeNode* root = create_tree();
    
    return 0;
}