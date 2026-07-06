#include "net.h"

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
int DataHandle(int fd, struct sockaddr_in *client_addr) {
    char buf[BUFSIZ] = {};
    int ret = recv(fd, buf, BUFSIZ, 0);
    
    if(ret < 0) {
        perror("recv");
        return ret;
    }
    
    if(ret > 0) {
        // 打印客户端IP、端口和接收到的数据
        printf("[%s:%d] received: %s", 
               inet_ntoa(client_addr->sin_addr), 
               ntohs(client_addr->sin_port), 
               buf);
    }
    return ret;
}