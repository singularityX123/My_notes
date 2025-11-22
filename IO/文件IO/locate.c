#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define N 128
int main(int argc, char const *argv[])
{   int fd = -1;
    int ret = 0;
    char buf[N] = {0};

    if (argc < 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    if (chmod(argv[1], 0200) < 0) // chmod改变文件权限  或者文件描述符 fchmod(fd, 0200)
    {
        printf("chmod error\n");
        return -1;
    }

    //文件不存在
    if (( fd = open(argv[1], O_RDWR|O_CREAT, 0664)) < 0)
    {
        printf("open error\n");
        return -1;
    }

    printf("open file success, fd = %d\n", fd);
    //刚打开文件，文件偏移量在文件头
    ret = lseek(fd, 0, SEEK_CUR); // 输出偏移量0
    printf("current offset: %d\n", ret);

    read(fd, buf, 10); 
    printf("%s\n", buf);
    ret = lseek(fd, 0, SEEK_CUR); // 
    printf("current offset: %d\n", ret);

    lseek(fd, 0, SEEK_END); //将文件偏移量移动到文件尾,否则会覆盖之前的数据

    write(fd, "hello", strlen("hello"));
    ret = lseek(fd, 0, SEEK_CUR); //
    printf("current offset: %d\n", ret);


    ret = lseek(fd, 0, SEEK_SET); //将文件偏移量移动到文件头
    printf("current offset: %d\n", ret);
    bzero(buf, sizeof(buf));
    while (read(fd, buf, sizeof(buf)) > 0)
    {
        printf("%s", buf);
        bzero(buf, sizeof(buf));
    }
    printf("Read file over\n");

    close(fd);
    return 0;
}