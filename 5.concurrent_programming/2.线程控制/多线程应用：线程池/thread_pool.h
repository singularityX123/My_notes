#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define THREAD_NUM 8

/*基本数据结构*/
typedef struct Task { //任务队列
    void (*function)(void *); // 任务函数（做什么）
    void *arg;                // 参数（用什么做）
    struct Task *next;        // 下一个任务
}task_t;

typedef struct thread_pool{ //线程池结构体
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_t tid[THREAD_NUM];
	task_t *task_head;
	task_t *task_tail;
	int shutdown;    //线程池的开关，0代表使用，1代表销毁 （为0，代表线程池可用，为1，代表线程池需要终止运行）
}tpool_t;


/*四个主要函数*/
// 工作线程
void *worker(void *arg); // 或者这种内部私有函数直接在.c中static实现，在此不声明

// 创建线程池
tpool_t *thread_pool_create();

// 添加任务到线程池, 添加成功返回0，失败返回-1
int thread_pool_add_task(tpool_t *pool, void (*function)(void *), void *arg); 

// 销毁线程池
int thread_pool_destroy(tpool_t *pool); 