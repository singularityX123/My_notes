#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//根据本机man手册
#include <fcntl.h>
#include <unistd.h>
#define N 25

int main(void)
{
    int fd = -1;
    //read
    //char buf[N] = {0};// 初始化是好习惯，防止野指针

    //write
    char buf[N] = "Hello!\n";

    int n = 0;
    int total = 0;
#if 1

    //read
    //if ((fd = open("1.txt", O_RDONLY)) < 0)
    if ((fd = open("1.txt", O_RDWR)) < 0) 
    {
        printf("open error!\n");
        return -1; // exit(1)是退出进程，return -1 是退出函数
    }
    printf("open success! file_name = 1.txt, fd = %d\n", fd);

    do{
        n = write(fd, buf, strlen(buf));
    }while(n < 0);
    printf("write success! n = %d\n", n);
    printf("Finished writing the file\n");

    #if 0
    while(( n = read(fd, buf, N)) > 0){// 从fd中读取N个字节到buf中，这里 = 0 会导致死循环，因为read 成功返回读取的字节数，失败返回-1，EOF返回0
        printf("%s\n", buf);
        total += n;
    } 
    printf("total_characters = %d\n", total);
    printf("Finished reading the file\n");
    #endif

#elif 0
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