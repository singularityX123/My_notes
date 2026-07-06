#include <stdio.h>
#define __USE_POSIX
#include <signal.h>
#include <unistd.h>
#include <string.h>



// 信号处理函数
void sighandler(int sig) {
    printf("捕获信号：%s\n", strsignal(sig) );
    // 可以在这里进行一些清理工作
    _exit(0);
}

int main() {
    struct sigaction sa;

    // 清空 sigaction 结构体
    sigemptyset(&sa.sa_mask);
    // 设置信号处理函数
    sa.sa_handler = sighandler;
    // 不使用信号处理函数的扩展功能
    sa.sa_flags = 0;

    // 使用 sigaction 注册信号处理函数
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    //SIGINT会终止nanosleep()系统调用，所以这里如果去掉while(1),会直接导致进程终止
    while (1) sleep(1); 

    return 0;
}