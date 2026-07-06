#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef __USE_XOPEN2K
    #define __USE_XOPEN2K
#endif
#include <netdb.h>     

#define PORT 80 // HTTP默认端口
#define DOMAIN_NAME "www.baidu.com" // 要解析访问的域名
#define BUILD_HTTP_REQUEST(host, path) \
    "GET " path " HTTP/1.1\r\n" \
    "Host: " host "\r\n" \
    "Connection: close\r\n" \
    "\r\n"
    
// 使用时:  char *request = BUILD_HTTP_REQUEST("www.baidu.com", "/"); 


int connect_to_host(const char *hostname, const char *port);
int main(int argc, char *argv[]){
    char *request = BUILD_HTTP_REQUEST(DOMAIN_NAME, "/"); 
    int sockfd = connect_to_host(DOMAIN_NAME, "80");
    char buf[BUFSIZ] = {};

    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("send");
        close(sockfd);
        return 1;
    }

    int bytes_received;
    while ((bytes_received = recv(sockfd, buf, BUFSIZ - 1, 0)) > 0) {
        buf[bytes_received] = '\0'; //安全字符串处理
        printf("%s", buf);
    }

    close(sockfd);

    return 0;
}

int connect_to_host(const char *hostname, const char *port){ 
    struct addrinfo hints, *res, *p;
    int sockfd = -1;
    int status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    } // 域名解析获取地址信息

    // 尝试返回所有的地址，直到连接成功
    for(p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol); // 得到的地址信息创建套接字
        if (sockfd == -1) {
            perror("socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) { // 尝试连接
            perror("connect");
            close(sockfd);
            continue;
        }
        break;
    }

    freeaddrinfo(res); // 释放地址信息链表（好习惯）

    if (p == NULL) {
        fprintf(stderr, "failed to connect\n");
        return -2;
    }

    return sockfd;
}

 

#if 0 // 参考代码: socket实现域名访问百度

    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #ifndef __USE_XOPEN2K
        #define __USE_XOPEN2K
    #endif
    #include <netdb.h>

    int main() {
    // 1. 获取百度的IP地址
    struct addrinfo hints, *res;
    int sockfd;
    char buffer[1024];
    ssize_t bytes_received;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo("www.baidu.com", "80", &hints, &res) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    // 2. 创建套接字并连接
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        freeaddrinfo(res);
        return 1;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        close(sockfd);
        freeaddrinfo(res);
        return 1;
    }

    freeaddrinfo(res);

    // 3. 发送HTTP GET请求
    const char *http_request = "GET / HTTP/1.1\r\n"
                            "Host: www.baidu.com\r\n"
                            "Connection: close\r\n\r\n";
    if (send(sockfd, http_request, strlen(http_request), 0) == -1) {
        perror("send");
        close(sockfd);
        return 1;
    }

    // 4. 接收并处理响应
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    if (bytes_received == -1) {
        perror("recv");
    }

    close(sockfd);
    return 0;

#endif