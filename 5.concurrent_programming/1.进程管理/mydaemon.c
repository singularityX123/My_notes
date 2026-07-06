#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define DEV_NULL "/dev/null"

int mydaemon(int nochdir, int noclose)
{
    pid_t pid;
    int fd;
    
    // 第一次fork，脱离父进程（会话组长）
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    } else if (pid > 0) {
        // 父进程退出
        _exit(EXIT_SUCCESS);
    }
    
    // 创建新会话，成为新会话的组长
    if (setsid() < 0) {
        perror("setsid");
        return -1;
    }
    
    // 第二次fork，确保不是会话组长，防止获得控制终端
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    } else if (pid > 0) {
        // 父进程（第一次fork的子进程）退出
        _exit(EXIT_SUCCESS);
    }
    
    // 设置文件创建掩码
    umask(0);
    
    // 改变工作目录
    if (!nochdir) {
        if (chdir("/") < 0) {
            perror("chdir");
            return -1;
        }
    }
    
    // 关闭所有打开的文件描述符
    if (!noclose) {
        long maxfd = sysconf(_SC_OPEN_MAX);
        if (maxfd < 0) {
            maxfd = 1024; // 默认值
        }
        
        for (fd = 0; fd < maxfd; fd++) {
            close(fd);
        }
        
        // 重定向标准输入、输出、错误到 /dev/null
        fd = open(DEV_NULL, O_RDWR);
        if (fd < 0) {
            perror("open " DEV_NULL);
            return -1;
        }
        
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2 stdin");
            return -1;
        }
        
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 stdout");
            return -1;
        }
        
        if (dup2(fd, STDERR_FILENO) < 0) {
            perror("dup2 stderr");
            return -1;
        }
        
        // 如果打开的文件描述符大于2，关闭它
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }
    
    // 设置信号处理
    // ...
    
    return 0;
}