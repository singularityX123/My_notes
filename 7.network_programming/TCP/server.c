#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#define BACKLOG 5 // 监听队列长度，平衡了并发处理能力和内存资源
#define BUFSIZ 8192 // 用于网络数据读取的缓冲区大小(固定长度会有内存局限性问题，实际应用中可动态分配内存)

void signal_handler(int signo){
    if (signo == SIGCHLD){
        printf("client %d exited\n", signo);
        wait(NULL); // 回收子进程资源，防止僵尸进程产生
    }
}
void client_handler(int client_fd, struct sockaddr_in client_addr); 
int main(int argc, char *argv[]){
    int fd;
    pid_t pid;
    struct sockaddr_in addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    /*设置SIGCHLD信号处理函数，处理子进程退出信号*/
    /* 使用 signal() 来避免对 struct sigaction 不完整类型的依赖 */
    if (signal(SIGCHLD, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    if (argc < 3){
        printf("Usage: %s <ip_address> <port>\n", argv[0]);
        exit(0);
    }

    // 创建套接字
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0){
        perror("socket");
        exit(0);
    }


    // 绑定通信结构体(地址信息)
    /*把地址信息（IP+端口）与套接字 fd 关联起来，让套接字在指定地址上监听连接*/
    addr.sin_family = AF_INET;
    addr.sin_port = htons( atoi(argv[2]) ); 
    //addr.sin_addr.s_addr = 0; // 等价于 INADDR_ANY（绑定到所有可用的网络接口，服务器接受来自任何IP地址的连接请求）
    if (inet_aton(argv[1], &addr.sin_addr)==0){
        fprintf(stderr, "Invalid IP address\n");
        exit(EXIT_FAILURE);
    }
    int flag = 1, len = sizeof(int); /*地址快速重用（设置套接字选项，允许地址重用）*/
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, len) < 0){
        perror("setsockopt");
        exit(1);
    }
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){ // 绑定
        perror("bind");
        exit(0);
    }


    // 监听客户端连接请求
    if (listen(fd, BACKLOG) < 0){
        perror("listen");
        exit(0);
    }
    printf("Server is listening on %s:%d\n", argv[1], atoi(argv[2]));

    // 接受多个客户端连接请求
    while(1){
        // TODO：一个accept对应处理一个请求，并发（处理多请求）时一边要accept一边fork
        // 服务端接受客户端连接请求(accept产生client的新套接字（文件描述符）用于数据交换)
        int client_fd = accept(fd, (struct sockaddr *)&client_addr, &addr_len); 
        if (client_fd < 0){
            perror("accept");
            exit(0);
        }

        // 获取客户端信息并打印
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        printf("Client %s:%d connected\n", ip_str, ntohs(client_addr.sin_port));

        // 多进程并发处理客户端请求
        if ((pid = fork()) < 0){
            perror("fork");// 出错
            close(client_fd);
            continue;

        } else if (pid == 0){ // 子进程
            close(fd); // 子进程关闭监听套接字，因为子进程不需要监听新的连接
            client_handler(client_fd, client_addr); // 处理客户端请求
            exit(0); // 处理完毕，该子进程退出

        } else {// 父进程
            close(client_fd); // （释放不必要资源）父进程关闭客户端套接字,因为处理任务已经交给子进程了

        }
    }
    
    close(fd); // 关闭服务端监听套接字

    return 0;
}

void client_handler(int client_fd, struct sockaddr_in client_addr){ 
    char client_ip[INET_ADDRSTRLEN] = "unknown_ip";
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    int client_port = ntohs(client_addr.sin_port);

    // 处理客户端发送的数据  
    while (1) {
        char buf[BUFSIZ] = {0};

        /*若开头初始化，可用一下方法清空缓冲区
         * memset(buf, 0, BUFSIZ); 
         * 或者 bzero(buf, BUFSIZ);
         */

        int ret = read(client_fd, buf, BUFSIZ);
        if (ret == 0) {
            printf("Client closed connection\n");
            break;
        } else if (ret < 0) {
            perror("read");
            break;
        } else {
            printf("Received from %s:%d: %s", client_ip, client_port, buf);
        }
    } 

    close(client_fd); // 关闭客户端套接字
}