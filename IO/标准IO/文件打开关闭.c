#include <stdio.h>
int main()
{
    FILE *fp = NULL; // 定义文件指针，初始化为空指针
    fp = fopen("buffered.txt", "r"); // 只读打开文件
    if (fp == NULL)
    {
        printf("文件打开失败");
        return -1;
    }

    printf("文件打开成功\n");
    fclose(fp); // 关闭文件，释放资源防止被其他程序占用（安全漏洞）
    fp = NULL; // 避免野指针
    return 0;
}