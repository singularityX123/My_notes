#include "../head/UDP.h"

/* 其他必要的环境已封装好了，只需要实现与客户端的交互即可
 * 这里的fd是服务端的socket，
 * addr是服务端的地址*/
extern void udp_main(const int fd, const char *argv) {
	char buf[BUFSIZ] = {};
	send(fd, argv, strlen(argv) + 1, 0);
	recv(fd, buf, BUFSIZ, 0);
	printf("buf=%s\n", buf);
}



