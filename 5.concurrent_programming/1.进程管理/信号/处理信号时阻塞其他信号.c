#include <stdio.h>
#define __USE_POSIX
#include <signal.h>
#include <unistd.h>
#include <string.h>

// 信号处理函数
void sigusr1_handler(int signum) {
    printf("捕捉到信号：%s\n", strsignal(signum) );
    sleep(5);  // 模拟长时间处理
    printf("阻塞完成\n");
}

int main() {
    struct sigaction sa;

    // 清空 sigaction 结构体
    sigemptyset(&sa.sa_mask);
    // 在处理 SIGUSR1 时阻塞 SIGUSR2
    sigaddset(&sa.sa_mask, SIGUSR2);
    // 设置信号处理函数
    sa.sa_handler = sigusr1_handler;
    // 不使用信号处理函数的扩展功能
    sa.sa_flags = 0;

    // 使用 sigaction 注册信号处理函数
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("进程ID:%d, 等待SIGUSR1信号...\n", getpid() );
    while (1) sleep(1);

    return 0;
}