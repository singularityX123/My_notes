/* glob在此代码中总是不能作为一个整体自增，老被其他线程修改？！*/

// 关键词：原子操作、同步机制、互斥量解决同步问题、竞态条件、自增运算的非原子性

//     解决方案
//互斥锁：软件层面同步
//原子操作：硬件层面支持
//无锁编程：复杂但高性能

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// TODO  初始化互斥量
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 第一次初始化互斥量，mutex锁为解锁状态

static int glob;  // 全局变量，被所有线程共享

// 线程函数
void *ThreadFunc(void *arg) {
    int loops = *((int *)arg);  // 从参数中获取循环次数
    int loc, j;
    
    /*关键问题，改为glob++仍然没用，因为glob++不是一个原子操作，硬件底层还是要分三步完成*/
    #if 0
    for(j = 0; j < loops; j++) { 
        loc = glob;  // 读取全局变量
        loc++;       // 局部变量自增
        glob = loc;  // 将局部变量的值赋给全局变量
    }
    #endif
    // TODO  加锁、临界区代码保护临界资源、解锁
    for (j=0; j<loops; j++){
        // 加锁
        pthread_mutex_lock(&mutex); // 若mutex锁为解锁状态，则加锁成功，若mutex锁为锁定状态，则**阻塞等待**直到mutex锁被解锁
        // 临界区代码，同一时刻只能有一个线程进入临界区
        glob++;
        
        // 解锁
        pthread_mutex_unlock(&mutex);
    }

    /*            glob++ 实际上等价于：
       int temp = glob;   // 步骤1：读取glob值到寄存器
       temp = temp + 1;   // 步骤2：寄存器的值进行计算
       glob = temp;       // 步骤3：将寄存器的值写回内存  */


    pthread_exit(NULL);  // 线程退出
}

int main(int argc, const char *argv[])
{
    pthread_t tid1, tid2;  // 定义两个线程 ID
    int loops;

    // 检查命令行参数
    if(argc != 2) {
        fprintf(stderr, "%s [loops]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 从命令行参数中读取循环次数
    if( sscanf(argv[1], "%d", &loops) != 1) {
        fprintf(stderr, "Invalid loops\n");
        exit(EXIT_FAILURE);
    }

    // 创建两个线程，都执行 ThreadFunc 函数
    pthread_create(&tid1, NULL, ThreadFunc, &loops);
    pthread_create(&tid2, NULL, ThreadFunc, &loops);

    // 等待两个线程结束
    pthread_join(tid1, NULL); 
    pthread_join(tid2, NULL);


    // TODO  销毁互斥量
    pthread_mutex_destroy(&mutex); // 销毁互斥量，释放相关资源
    
    // 打印最终的全局变量值
    printf("glob = %d\n", glob);
    return 0;
}