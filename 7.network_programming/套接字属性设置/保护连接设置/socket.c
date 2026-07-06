#include "net.h"
 
// 检查参数是否小于3
void Argment(int argc, char *argv[]){
    if(argc < 3){
        fprintf(stderr, "%s<addr><port>\n", argv[0]);
        exit(0);
    }
}
int CreateSocket(char *argv[]){
    /*创建套接字*/
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
        ErrExit("socket");
    /*允许地址快速重用*/
    int flag = 1;
    if( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag) ) )
        perror("setsockopt");
    /*设置通信结构体*/
    Addr_in addr;
    bzero(&addr, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( atoi(argv[2]) );
    /*绑定通信结构体*/
    if( bind(fd, (Addr *)&addr, sizeof(Addr_in) ) )
        ErrExit("bind");
    /*设置套接字为监听模式*/
    if( listen(fd, BACKLOG) )
        ErrExit("listen");
    return fd;
}
int DataHandle(int fd){
    char buf[BUFSIZ] = {};
    Addr_in peeraddr;
    socklen_t peerlen = sizeof(Addr_in);
    if( getpeername(fd, (Addr *)&peeraddr, &peerlen) )
        perror("getpeername");
    int ret = recv(fd, buf, BUFSIZ, 0);
    if(ret < 0)
        perror("recv");
    if(ret > 0){
        printf("[%s:%d]data: %s", 
                inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port), buf);
    }
    return ret;
}