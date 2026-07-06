#include "thread_pool.h"

#if 0
/*线程池功能测试*/
void cook(void *arg) { // 添加的任务函数
    int order_id = *(int *)arg;
    printf("厨师%lu 开始制作订单%d\n", pthread_self(), order_id);
    sleep(1); // 模拟烹饪时间
    printf("订单%d 完成！\n", order_id);
}
 
int main(int argc, const char *argv[])
{
    // 创建线程池
    tpool_t *pool = thread_pool_init();
    if(pool == NULL) {
    }
 
    // 添加任务
    int orders[10];
    for(int i=0; i<10; i++) {
        orders[i] = 1000 + i;
        thread_pool_add_task(pool, cook, &orders[i]);
    }
    sleep(4);
    // 销毁线程池
    thread_pool_destroy(pool);
    return 0;
}
#endif

#if 1 // 模拟订餐系统
void cook(void *arg) {
    int order_id = *(int *)arg;
    printf("厨师%lu 开始制作订单%d\n", pthread_self(), order_id);
    sleep(1); // 模拟烹饪时间
    printf("订单%d 完成！\n", order_id);
}



int main(int argc, const char *argv[]) {
    tpool_t *kitchen = thread_pool_create();
    
    // 模拟10个订单
    int orders[10];
    for(int i=0; i<10; i++) {
        orders[i] = 1000 + i;
        thread_pool_add_task(kitchen, cook, &orders[i]);
    }
    
    sleep(5); // 等待所有订单完成
    thread_pool_destroy(kitchen);
    return 0;
}
#endif