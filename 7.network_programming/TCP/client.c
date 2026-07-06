/*************************************************
 *TEST 默认情况下，系统测试ip只有 127.0.0.1 是激活的*
 *************************************************/
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]){
    if (argc < 3){  
        printf("Usage: %s <ip_address> <port>\n", argv[0]);
        exit(0);
    }

    int fd;
    struct sockaddr_in addr;
    
    // 创建套接字
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0){
        perror("socket");
        exit(0);
    }

    // 设置服务器地址
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));  // 端口号
    if (inet_aton(argv[1], &addr.sin_addr) == 0){  // IP地址
        fprintf(stderr, "Invalid IP address\n");
        exit(EXIT_FAILURE);
    }
    
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("connect");
        exit(0);
    }

    // 发送数据到服务端
    char buffer[1024];  // 使用可修改的缓冲区
    while(1){
        printf("Input-> ");
        fflush(stdout);  // 刷新输出缓冲区
        
        // 检查fgets返回值
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;  // 用户输入EOF或错误
        }
        
        // 检查write返回值
        ssize_t bytes_written = write(fd, buffer, strlen(buffer));
        if (bytes_written < 0) {
            perror("write");
            break;
        }
    }
    
    // 关闭套接字
    close(fd);
    return 0;
}