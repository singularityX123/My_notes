/*域名解析，将主机名（域名）转换为IP地址
    ⚠️ 已废弃函数警告：gethostbyname() 函数已被认为过时，
    在现代网络编程中推荐使用 getaddrinfo() 函数替代，
    因为它支持IPv6并具有更好的错误处理机制。
*/
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>


int main(int argc, char *argv[])
{   
    int i;
    if (argc < 2){
        printf("Usage: %s <hostname>\n", argv[0]);
        exit(0);
    }

    struct hostent *host = gethostbyname(argv[1]);

    for (i = 0; host->h_aliases[i] != NULL; i++){
        printf("%s\n", host->h_aliases[i]);
    }
    printf("Address Type: %s\n", host->h_addrtype == AF_INET ? "IPv4" : "IPv6");

    for (i = 0; host->h_addr_list[i] != NULL; i++){
        printf("IP Address %d: %s\n", i + 1, inet_ntoa(*(struct in_addr *)host->h_addr_list[i]));
    }

    endhostent();
    return 0;
}