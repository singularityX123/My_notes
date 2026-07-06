#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_READERS 5
#define NUM_WRITERS 2

int balance = 1000; // 初始账户余额
pthread_rwlock_t rwlock; // 读写锁

// 读取者线程函数
void* reader(void* arg) {
    int id = *((int*)arg);
    while (1) {
        pthread_rwlock_rdlock(&rwlock); // 获取读锁
        printf("读者 %d: 当前余额为 %d 元\n", id, balance);
        pthread_rwlock_unlock(&rwlock); // 释放读锁

        sleep(rand() % 3); // 随机休眠
    }
    return NULL;
}

// 写入者线程函数
void* writer(void* arg) {
    int id = *((int*)arg);
    while (1) {
        int amount = rand() % 100; // 随机生成一个金额
        pthread_rwlock_wrlock(&rwlock); // 获取写锁
        balance += amount; // 修改余额
        printf("写者 %d: 存入 %d 元，最新余额为 %d 元\n", id, amount, balance);
        pthread_rwlock_unlock(&rwlock); // 释放写锁

        sleep(rand() % 3); // 随机休眠
    }
    return NULL;
}

int main() {
    pthread_t readers[NUM_READERS];
    pthread_t writers[NUM_WRITERS];
    int reader_ids[NUM_READERS];
    int writer_ids[NUM_WRITERS];

    srand(time(NULL)); // 初始化随机数种子

    // 初始化读写锁
    pthread_rwlock_init(&rwlock, NULL);

    // 创建读取者线程
    for (int i = 0; i < NUM_READERS; i++) {
        reader_ids[i] = i + 1;
        pthread_create(&readers[i], NULL, reader, &reader_ids[i]);
    }

    // 创建写入者线程
    for (int i = 0; i < NUM_WRITERS; i++) {
        writer_ids[i] = i + 1;
        pthread_create(&writers[i], NULL, writer, &writer_ids[i]);
    }

    // 等待读取者线程结束（实际上不会结束）
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }

    // 等待写入者线程结束（实际上不会结束）
    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writers[i], NULL);
    }

    // 销毁读写锁
    pthread_rwlock_destroy(&rwlock);

    return 0;
}