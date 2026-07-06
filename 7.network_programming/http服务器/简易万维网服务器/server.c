#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <strings.h>
 
#define PORT 80
#define BACKLOG 5
#define HTTPFILE "http_head.txt"
#define HTMLFILE "home_response.html"
 
int ClientHandle(int newfd);
 
int main(int argc, char *argv[])
{   
    printf("server start...\n");
    int fd, newfd;
    struct sockaddr_in addr;
    /*创建套接字*/
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0){
        perror("socket");
        exit(0);
    }
    int opt = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt, sizeof(opt) ))
        perror("setsockopt");
 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = 0;
    /*绑定通信结构体*/
    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr) ) == -1){
        perror("bind");
        exit(0);
    }
    /*设置套接字为监听模式*/
    if(listen(fd, BACKLOG) == -1){
        perror("listen");
        exit(0);
    }
    /*接受客户端的连接请求，生成新的用于和客户端通信的套接字*/
    newfd = accept(fd, NULL, NULL);
    if(newfd < 0){
        perror("accept");
        exit(0);
    }

    
    ClientHandle(newfd);

    close(fd);
    return 0;
}
 
int ClientHandle(int newfd){
    int file_fd = -1; // 初始化为无效值
    char buf[BUFSIZ] = {};
    int ret;
 
    do {
        ret = recv(newfd, buf, BUFSIZ, 0);
    }while(ret < 0 && errno == EINTR);
    if(ret < 0){
        perror("recv");
        exit(0);
    }else if(ret == 0){
        close(newfd);
        return 0;
    }else{
        printf("=====================================\n");
        printf("%s", buf);
        fflush(stdout); // 强制刷新输出缓冲区
    }
 
    bzero(buf, ret);
    file_fd = open(HTTPFILE, O_RDONLY);
    if(file_fd < 0){
        perror("open");
        exit(0);
    }
    ret = read(file_fd, buf, BUFSIZ);
    printf("%s\n", buf);
    send(newfd, buf, ret, 0);
    close(file_fd);
 
    bzero(buf, ret);
    file_fd = open(HTMLFILE, O_RDONLY);
    if(file_fd < 0){
        perror("open");
        exit(0);
    }
    ret = read(file_fd, buf, BUFSIZ);
    printf("%s\n", buf);
    send(newfd, buf, ret, 0);
    close(file_fd);
 
    close(newfd);
    printf("client disconnected\n");

    return 0;
}