#include "../head/UDP.h"

extern void udp_main(const int fd, const char *argv) ;

int main(int argc, char *argv[])
{ 
#if 1 // 文件IO输入PORT和HOST

    /* 文件IO输入PORT和HOST */
    char host[256] = {0};  // 分配固定大小的数组
    char port[256] = {0};
    FILE *fp = fopen(".config", "r");
    printf("正在从.config文件中读取服务器的主机IP和端口号...\n");

    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    // 读取主机IP
    if (fgets(host, sizeof(host), fp) == NULL) {
        printf("读取host失败\n");
        fclose(fp);
        return EXIT_FAILURE;
    }
    host[strcspn(host, "\n")] = '\0'; // 去除换行符
    
    // 读取端口号
    if (fgets(port, sizeof(port), fp) == NULL) {
        printf("读取port失败\n");
        fclose(fp);
        return EXIT_FAILURE;
    }
    port[strcspn(port, "\n")] = '\0'; // 去除换行符

    fclose(fp);

    /* 检查参数, 其中第二个参数是需要翻译的单词 */
    if(argc < 2) {
        printf("Usage: %s [word]\n", argv[0]);  // 修改了输出格式
        exit(EXIT_FAILURE);
    }
    
    /* 打印host和port的值 */
    printf("服务器的主机IP是%s, 端口号是%s\n", host, port);
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0)
        ErrExit("socket");

    /* 设置通信结构体 */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    if(inet_aton(host, &addr.sin_addr) == 0) {
        printf("[%s:%d] Invalid address\n", __FUNCTION__, __LINE__);
        exit(EXIT_FAILURE);
    }

    /* 发起连接请求,注意UDP连接没有三次握手, 不存在连接失败, 只是确定接受端而已 */
    if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)  // 添加了<0判断
        ErrExit("connect");

    /* 执行客户端处理程序 */
    udp_main(fd, argv[1]);
    
    /* 关闭套接字 */
    close(fd);

#else  // 设置环境变量输入PORT和HOST

    /* 获取环境变量DICTIONARY_SERVER_PORT */
	char *port = getenv("DICTIONARY_SERVER_PORT");
	if(port == NULL) {
		printf("没有发现环境变量[DICTIONARY_SERVER_PORT]\n");
		exit(EXIT_FAILURE);
	}

	/* 获取环境变量DICTIONARY_SERVER_HOST */
	char *host = getenv("DICTIONARY_SERVER_HOST");
	if(host == NULL) {
		printf("没有发现环境变量[DICTIONARY_SERVER_HOST]\n");
		exit(EXIT_FAILURE);
	}

	/* 检查参数, 其中第二个参数是需要翻译的单词 */
	if(argc < 2) {
		printf("[%s][word]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	/* 打印环境变量的值 */
	printf("服务器的主机IP是%s, 端口号是%s\n", host, port);
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
		ErrExit("socket");

	/* 设置通信结构体 */
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons( atoi(port) );
	if( inet_aton( host, &addr.sin_addr) == 0) {
		printf("[%s:%d] Invalid address\n", __FUNCTION__, __LINE__);
		exit(EXIT_FAILURE);
	}

	/* 发起连接请求,注意UDP连接没有三次握手, 不存在连接失败, 只是确定接受端而已 */
	if(connect(fd, (struct sockaddr *)&addr, sizeof(addr) ) )
		ErrExit("connect");

	/* 执行客户端处理程序 */
	udp_main(fd, argv[1]);
	
	/* 关闭套接字 */
	close(fd);
    
#endif


    return 0;
}