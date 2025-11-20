#include <stdio.h>
#include <stdlib.h>
//根据本机man手册
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd = -1;
#if 0
    if ((fd = open("1.txt", O_RDONLY)) < 0)
    {
        printf("open error!\n");
        return -1; // exit(1)是退出进程，return -1 是退出函数
    }
    printf("open success! file_name = 1.txt, fd = %d\n", fd);

#elif 1
    if ((fd = open("2.txt", O_CREAT|O_RDONLY, 0664)) < 0) // 0664 表示文件所有者、文件所在组、其他用户都有读写权限
    {
        printf("open error!\n");
        return -1; 
    }
    printf("open success! file_name = 2.txt, fd = %d\n", fd);
    
#endif
    if (close(fd) == -1) // close 成功返回0，失败返回-1
    {
        printf("close error! fd = %d\n", fd);
        return -1; 
    }
    fd = -1; // 关闭文件后，fd修改为 -1
    printf("close success! fd = %d\n", fd);

    return 0;
}