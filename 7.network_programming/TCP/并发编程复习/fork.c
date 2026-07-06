/*创建子进程执行任务*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    pid_t pid = fork(); // fork（）一次后有了两个进程，后续代码都会被执行两次(当然，父与子不完全一样)

    if (pid < 0) {
        perror("fork");
        exit(0); // 程序异常 退出程序
    } else if (pid == 0) {
        // Child process
        printf("This is child process.\n");
    } else {
        // Parent process
        printf("This is parent process.\n");
        wait(NULL); // 父进程阻塞等子进程结束 防止孤儿进程
    }

    /*创建需要执行的任务
     * 各个进程根据自己的身份，在目前基础上打开、关闭资源，进行<分化>
     */


    return 0;
}