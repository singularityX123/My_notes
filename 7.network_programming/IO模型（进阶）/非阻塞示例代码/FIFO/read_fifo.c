#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#define NONBLOCK
#define FIFO_NAME "/tmp/myfifo" // 有名管道文件路径

int main(int argc, char *argv[]) {
    int fd, ret;
    char buf[BUFSIZ] = {};
 
    // 创建有名管道
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("mkfifo");
        exit(0);
    }
 
    fd = open(FIFO_NAME, O_RDONLY|O_NONBLOCK); // O_NONBLOCK 标志位，文件描述符工作方式变为非阻塞
    if(fd < 0) {
        perror("open");
        exit(0);
    }
 
    while(1) {

    #ifdef BLOCK
     do {
        ret = read(fd, buf, BUFSIZ); 
     } while (ret < 0 && errno == EAGAIN);

     if(ret < 0){
         perror("read");
         exit(0);
     }

    #elif defined NONBLOCK // 阻塞多线程  非阻塞可轮询

        ret = read(fd, buf, BUFSIZ); // 直接向recvfrom返回（一直重复到数据报准备好，拷贝数据报），非阻塞不会等待写进程向 /tmp/myfifo 管道写入数据

    #endif

        if(buf[0] == '#')
            break;
        printf("Read from pipe: %s\n", buf);
        memset(buf, 0, BUFSIZ);
        sleep(2); 
    }

    // 关闭管道并删除有名管道文件
    close(fd);
    unlink(FIFO_NAME);
    
 
    return 0;
}

