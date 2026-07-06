#include "../head/UDP.h"

extern void UDP_main(const int fd, const struct sockaddr_in *addr);
int main(int argc,const char *argv[])
{ 
    /* 1.检查参数 */
	if(argc < 3) {
		printf("[%s][addr][port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* 2.创建数据报套接字 */
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
		ErrExit("socket");

	/* 3.设置通信结构体 */
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( atoi(argv[2]) );
	if( inet_aton(argv[1], &addr.sin_addr) == 0) {
		printf("[%s:%d] Invalid address\n", __FUNCTION__, __LINE__);
		exit(EXIT_FAILURE);
	}

	/* 4.绑定通信结构体 */
	if( bind(fd, (struct sockaddr *)&addr, sizeof(addr) ) )
		ErrExit("bind");

	/* 5.处理客户端数据 */
	UDP_main(fd, &addr);
	
	/* 6.关闭套接字 */
	close(fd);
	return 0;
}