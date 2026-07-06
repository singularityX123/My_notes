# 使用socket直接实现http通信

#### 第一步：实现socket通信

```c
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>

int main(int argc, const char *argv[])
{
    // 创建一个套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        perror("socket");  // 如果套接字创建失败，输出错误信息
        return -1;
    }
    
    // 定义 sockaddr_in 结构体以指定绑定地址
    struct sockaddr_in addr = {
        .sin_family = AF_INET,              // 使用 IPv4
        .sin_port = htons(8080),            // 指定端口号 8080
        .sin_addr.s_addr = INADDR_ANY       // 绑定到所有可用的网络界面
    };
    
    // 将套接字绑定到指定地址和端口
    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) ) {
        perror("bind");  // 如果绑定失败，输出错误信息
        return -2;
    }
    
    // 使套接字进入监听状态，最多接受 5 个未处理的连接请求
    if(listen(fd, 5) ) {
        perror("listen");  // 如果监听失败，输出错误信息
        return -3;
    }
    
    // 接受客户端连接请求
    int cfd = accept(fd, NULL, NULL);  // 接受连接请求
    if(cfd < 0) {
        perror("accept");
        return -4;
    }

    while(1) {
        char buf[BUFSIZ] = {};  // 定义一个缓冲区，用于存放接收到的数据
        int r = recv(cfd, buf, BUFSIZ, 0);  // 接收来自客户端的数据
        if(r < 0) {
            perror("recv");  // 如果接收失败，输出错误信息
            return -5;
        } else if(r == 0) {
            break;  // 如果接收到的数据长度为 0，则说明客户端已关闭连接，退出循环
        } else {
            printf("%s\n", buf);  // 打印接收到的数据
        }
    }

    // 关闭客户端和服务器套接字写入通道
    shutdown(cfd, SHUT_WR);  // 关闭客户端套接字的写通道
    shutdown(fd, SHUT_WR);   // 关闭服务器套接字的写通道
    return 0;
}
```

#### 第二步：判断并处理GET请求

```c
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

//判断文件后缀
char *get_extension(char *filename) {
	char *dot = strrchr(filename, '.');
	if(!dot || dot == filename) {
		return "html";
	}
	return dot+1;
}

//获取系统时间
char *get_time(char *time_str, int len) {
    time_t rawtime;
    struct tm *timeinfo;

    // 获取当前的 UTC 时间
    time(&rawtime);
    timeinfo = gmtime(&rawtime);

    // 格式化时间字符串
    strftime(time_str, len, "Date: %a, %d %b %Y %H:%M:%S GMT", timeinfo);
    return time_str;
}

//发送数据
ssize_t send_data(int fd, const void *buf) {
	int ret = send(fd, buf, strlen(buf), 0);
	if(ret < 0) {
		perror("send");
		exit(EXIT_FAILURE);
	}
	return ret;
}

//响应get请求
int do_get(int fd, char *buf, size_t len) {
	char type[16] = {};
	char resource[16] = {};
	char head[1024] = {};
	char *pathname = NULL;
	int ret = sscanf(buf, "%s%s\n", type, resource);
	if(ret != 2) {
		char *head = "HTTP/1.1 400 Bad Request\r\n\r\n";
		return send_data(fd, head);
	}
	printf("type = %s  resource = %s\n", type, resource);

	if(strncasecmp("GET", type, 3) ) {
		char *head = "HTTP/1.1 501 Not Implemented\r\n\r\n";
		return send_data(fd, head);
	}
	if( strlen(resource) == 1 && resource[0] == '/') {
		pathname = "index.html";
	} else {
		pathname = &resource[1];
	}

	FILE *fp = fopen(pathname, "r");
	if(fp == NULL ){
		char *head = "HTTP/1.1 404 Not Found\r\n\r\n";
		return send_data(fd, head);
	}
	fread(buf, 1, len, fp);
	char time[100] = {};
	sprintf(head, "HTTP/1.1 200 OK\r\n"
			"%s\r\n"
			"Content-Length: %ld\r\n"
			"Connection: Keep-Alive\r\n" 
			"Content-Type: text/%s;charset=UTF-8\r\n\r\n", 
			get_time(time, 100), strlen(buf) , get_extension(pathname) );

	printf("send %s %s", head, buf);
	send_data(fd, head);
	return send_data(fd, buf);
}

int main(int argc, const char *argv[])
{
    ...
	//省略重复代码
	while(1) {
		int newfd = accept(fd, NULL, NULL);
		if(newfd < 0) {
        	perror("accept");
        	return -4;
		}
		while(1) {
			char buf[BUFSIZ] = {};
			int ret = recv(newfd, buf, BUFSIZ, 0);
			if(ret < 0) {
				perror("recv");
				return -5;
			} else if(ret > 0) {
				printf("%s\n", buf);
				do_get(newfd, buf, BUFSIZ);
			} else {
				close(newfd);
				break;
			}
		}
	}
	close(fd);
	return 0;
}
```

#### 注意：

1. 可以`shutdown()`可以替代`close()`，但是`shutdown()`不会真正关闭套接字，只是关闭读端或者写端

2. 如果在客户端关闭之前关闭服务端，马上再次运行程序，可能会出现端口被占用的错误

   -- 对此可以使用`setsockopt()`设置端口复用，设置`SOL_SOCKET`级别的`SO_REUSEADDR`属性：

   ```
   // 设置端口复用
       if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
           perror("setsockopt");
           exit(EXIT_FAILURE);
       }
   ```

   
