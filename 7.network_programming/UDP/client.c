#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
  
int main(int argc, char *argv[])
{
    int fd;
    struct sockaddr_in addr;
    char buf[BUFSIZ] = {0};
    socklen_t addrlen = sizeof(addr);
    if(argc < 3){
        fprintf(stderr, "%s<addr><port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    /*创建套接字*/
    if( (fd = socket(AF_INET, SOCK_DGRAM, 0) ) < 0){
        perror("socket");
        exit(EXIT_FAILURE);
    }
    /*设置通信结构体*/
    bzero(&addr, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( atoi(argv[2]) );
    if(inet_aton(argv[1], &addr.sin_addr) == 0) {
        fprintf(stderr, "Invalid address\n");
        exit(EXIT_FAILURE);
    }
    while(1){
        bzero(buf, BUFSIZ);
        printf("Input->");
        fgets(buf, BUFSIZ, stdin);
        sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&addr, addrlen);
    }
    close(fd);
    return 0;
}