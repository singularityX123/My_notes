#include "net.h"
#include <sys/select.h>
#define MAX_SOCK_FD 1024
 
int main(int argc, char *argv[])
{   
    // select函数模型服务器核心变量声明
    /* @param fd 用于监听套接字文件描述符
     * @param newfd 接收客户端连接的新描述符
     * @param set 描述符集合，维护当前所有活跃的套接字
     * @param tmpset 临时描述符集合 每次select调用前从set复制*/
    int i, ret, fd, newfd;
    fd_set set, tmpset;
    Addr_in clientaddr; // 客户端地址
    socklen_t clientlen = sizeof(Addr_in); // 客户端地址结构长度，初始化为Addr_in大小


    /*检查参数，小于3个 直接退出进程*/
    Argment(argc, argv);
    /*创建已设置监听模式的套接字*/
    fd = CreateSocket(argv);
 
    /*初始化描述符集合*/
    FD_ZERO(&set);
    FD_ZERO(&tmpset);
    FD_SET(fd, &set); // 将监听套接字加入集合
    
    while(1){
        tmpset = set;
        if( (ret = select(MAX_SOCK_FD, &tmpset, NULL, NULL, NULL)) < 0)
            ErrExit("select");
        if(FD_ISSET(fd, &tmpset) ){ // 监听套接字有事件发生
            /*接收客户端连接，并生成新的文件描述符*/
            if( (newfd = accept(fd, (Addr *)&clientaddr, &clientlen) ) < 0)
                perror("accept");
            printf("[%s:%d]connected\n", 
                    inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
            FD_SET(newfd, &set);
        }else{ //处理客户端数据
            for(i = fd + 1; i < MAX_SOCK_FD; i++){
                if(FD_ISSET(i, &tmpset)){
                    if( DataHandle(i) <= 0){
                        if( getpeername(i, (Addr *)&clientaddr, &clientlen) )
                            perror("getpeername");
                        printf("[%s:%d]disconnected\n", 
                                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                        FD_CLR(i, &set);
                        close(i);
                    }
                }
            }
        }
    }
    return 0;
}