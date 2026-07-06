#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>

void *func (void *arg)
{   
    printf("新线程ID: %ld\n", pthread_self());
    printf("新线程正在执行任务...\n");
    sleep(2);  // 模拟工作
    printf("新线程结束\n");
    return NULL;
}

int main(int argc, char const *argv[])
{
    pthread_t thread_id;
    int ret = pthread_create(&thread_id, NULL, func, NULL);

    if(ret != 0) {
        printf("pthread_create error: %s\n", strerror(ret));
        return 1;
    }

    printf("主线程创建的线程ID: %ld\n", thread_id);
    printf("主线程ID: %ld\n", pthread_self());
    
    // 关键：等待新线程结束
    pthread_join(thread_id, NULL);
    
    printf("主线程结束\n");
    return 0;
}