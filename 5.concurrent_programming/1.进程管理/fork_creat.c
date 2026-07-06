#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char const *argv[])
{   
    #if 0 // 简单双进程
    pid_t pid = fork(); // 创建子进程  单一程序下可以多进程了
    printf("Hello World!\n");
    /* 运行此程序后，会输出两次 "Hello World!"
    *  因为 fork() 会创建一个子进程，子进程会复制父进程的所有代码和数据，
    *  包括 printf("Hello World!\n"); 这行代码。
    *  所以，子进程会输出 "Hello World!"，而父进程也会输出 "Hello World!"。
    *  终端下有三个进程：父进程、子进程和 shell 进程。
    */

    if (pid == 0)
    {
        printf("子进程：%d\n", getpid());
    }
    else if (pid > 0)
    {
        printf("父进程：%d\n", getpid());
    }
    else
    {
        printf("创建子进程失败\n");
    }
    printf("进程ID：%d\n", getpid());
    #endif 

    #if 1 // 一父多同级子进程
    for (int i = 0; i < 3; i++)
    {
        pid_t pid = fork(); // 创建子进程  单一程序下可以多进程了
        if (pid == 0)
        {
            printf("子进程：%d\n", getpid());
            break; // 子进程只执行一次，避免节外生枝
        }
        else if (pid > 0)
        {
            printf("父进程：%d\n", getpid());
        }
        else
        {
            printf("创建子进程失败\n");
        }
    }
    while(1){
        sleep(1);
    }
    
    // ./fork_creat &  后台运行
    // 终端运行 pstree -p 查看进程树

    #endif


    return 0; // --> 在main函数中等价于 exit(0);  return n 就是系统调用了 exit(n);
              //     在普通函数中，return n 只是从该函数返回，不会结束程序；exit(n) 直接结束程序。
}


