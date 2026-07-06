#include "net.h"

int main(int argc, char *argv[])
{   
    int fd, newfd, i, j;
    nfds_t nfds = 1;
    struct pollfd fds[MAX_SOCK_FD] = {};
    Addr_in addr; // 用于accept时接收客户端地址
    socklen_t addrlen = sizeof(Addr_in);
    // 存储所有客户端的地址信息
    struct sockaddr_in client_addrs[MAX_SOCK_FD] = {0};

    /*检查参数，小于3个 直接退出进程*/
    Argment(argc, argv);
    /*创建已设置监听模式的套接字*/
    fd = CreateSocket(argv);

    fds[0].fd = fd; //
    fds[0].events = POLLIN; // 监听可读事件

    while (1){
        if (poll(fds, nfds, -1) < 0)
            ErrExit("poll");

        for(i = 0; i < nfds; i++){
            if (fds[i].fd == fd && fds[i].revents & POLLIN){ // 文件描述符有数据且可读
                /*接收客户端连接，并生成新的文件描述符*/
                newfd = accept(fd, (Addr *)&addr, &addrlen);
                if(newfd < 0)
                    perror("accept");

                printf("[%s:%d][nfds = %ld] connected\n", 
                       inet_ntoa(addr.sin_addr), 
                       ntohs(addr.sin_port), 
                       nfds);
                
                if (nfds >= MAX_SOCK_FD){
                    close(newfd); 
                    printf("Too many connections, rejecting...\n");
                    continue;
                }
            
                // 保存客户端地址信息
                client_addrs[nfds] = addr;
                
                // 添加新文件描述符
                fds[nfds].fd = newfd;
                fds[nfds].events = POLLIN;
                nfds++;

            }else if ( fds[i].revents & POLLIN){ //客户端有数据了
                /*处理客户端数据*/
                if (DataHandle(fds[i].fd, &client_addrs[i]) <= 0){
                    printf("[%s:%d] disconnected\n",
                           inet_ntoa(client_addrs[i].sin_addr),
                           ntohs(client_addrs[i].sin_port));
                    
                    close(fds[i].fd);
                    // 从数组中移除该文件描述符
                    for(j = i; j < nfds - 1; j++){
                        fds[j] = fds[j + 1];
                        client_addrs[j] = client_addrs[j + 1];
                    }
                    nfds--;
                    i--;
                }
            }
        }
    }
    close(fd);
    return 0;
}