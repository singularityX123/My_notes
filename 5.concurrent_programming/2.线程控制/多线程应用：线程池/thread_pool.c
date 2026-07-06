#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "thread_pool.h"


void *worker(void *arg) {
    tpool_t *pool = (tpool_t *)arg;
    while(1) {
        //1. 锁定临界资源
        pthread_mutex_lock(&pool->mutex);
        //2. 等待任务
        while(pool->task_head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }
        //3. 检查线程开关
        if(pool->shutdown) {
            pthread_mutex_unlock(&pool->mutex);
            pthread_exit(NULL);
        }
        //4. 取出任务
        task_t *task = pool->task_head;
        if(task == NULL) {
            pthread_mutex_unlock(&pool->mutex);
            continue;
        }
        //5. 列新任务队列
        pool->task_head = task->next;
        if(pool->task_head == NULL) {
            pool->task_tail = NULL;
        }
        //6. 解锁
        pthread_mutex_unlock(&pool->mutex);
        //7. 执行任务
        task->function(task->arg);
        free(task);
    }
    return NULL;
}


tpool_t *thread_pool_create() {
    //1. 为线程池结构体申请内存
    tpool_t *pool = malloc(sizeof(tpool_t));
    if(pool == NULL) {
        return NULL;
    }
 
    //2. 初始化互斥量，条件变量
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);
 
    //3. 初始化队列
    pool->task_head = NULL;
    pool->task_tail = NULL;
 
    //4. 初始化线程池的开关
    pool->shutdown = 0;
 
    //5. 创建工作线程
    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_create(&pool->tid[i], NULL, worker, pool);
    }
    return pool;
}


int thread_pool_add_task(tpool_t *pool, void (*function)(void *), void *arg) {
    //1. 创建新任务
    task_t *new_task = malloc(sizeof(task_t));
    if(new_task == NULL) {
        return -1;
    }
    new_task->function = function;
    new_task->arg = arg;
    new_task->next = NULL;
 
    //2. 锁定临界资源
    pthread_mutex_lock(&pool->mutex);
    //3. 加入队列
    if(pool->task_tail == NULL) {
        pool->task_head = pool->task_tail = new_task;
    } else {
        pool->task_tail->next = new_task;
        pool->task_tail = new_task;
    }
 
    //4. 通知工作线程，并解锁
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}
 

int thread_pool_destroy(tpool_t *pool) {
    //1. 关闭线程池
    pthread_mutex_lock(&pool->mutex);
    pool->shutdown = 1;
    pthread_mutex_unlock(&pool->mutex);
 
    //2. 唤醒所有等待的线程
    pthread_cond_broadcast(&pool->cond);
 
    //3. 等待所有线程退出
    for(int i = 0; i < THREAD_NUM; i++) {
        pthread_join(pool->tid[i], NULL);
    }
     
    //4. 释放各种资源
    task_t *p = pool->task_head;
    while(p != NULL) {
        task_t *temp = p;
        p = p->next;
        free(temp);
    }
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    
    free(pool);
    return 0;
}
 
