#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
 
#define FIFO_NAME "/tmp/myfifo"
 
int main(void) {
    int fd;
    char buf[BUFSIZ];
 
    // 打开有名管道并进行读写操作
    fd = open(FIFO_NAME, O_WRONLY);
    if( fd < 0 ) {
        perror("open");
        exit(0);
    }
    while(1) {
        fgets(buf, BUFSIZ, stdin);
        if (write(fd, buf, BUFSIZ) < 0 ) {
            perror("write");
            exit(0);
        }
        if(buf[0] == '#')
            break;
    }
 
    close(fd);
    return 0;
}