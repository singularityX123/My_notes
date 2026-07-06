#include "net.h"

int main(int argc, char *argv[])
{
	/*检查参数，小于3个 直接退出进程*/
	Argment(argc, argv);
	/*创建已设置监听模式的套接字*/
	int fd = CreateSocket(argv);
	/*接收客户端连接，并生成新的文件描述符*/
	int newfd = accept(fd, NULL, NULL);
	if(newfd < 0)
		perror("accept");
	/*处理客户端数据*/
	while(DataHandle(newfd) > 0);
	return 0;
}
