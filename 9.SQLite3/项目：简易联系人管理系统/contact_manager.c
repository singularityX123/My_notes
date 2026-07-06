#include <stdio.h>
int choice;

int main(int argc, char* argv[]) {





    while (1)
    {
        printf("请输入执行操作：\n");
        printf("1. 添加联系人\n");
        printf("2. 查找联系人\n");
        printf("3. 更新联系人\n");
        printf("4. 删除联系人\n");
        printf("5. 退出程序\n");
        while (!scanf("%d", &choice)) getchar();

        switch (choice)
        {
        case 1:
            printf("联系人姓名\n");
            scanf("%s", name);
            getchar();
            printf("联系人电话\n");
            scanf("%s", phone);
            return 0;
        
        default:
            break;
        }
        
    }
    




  return 0;
}