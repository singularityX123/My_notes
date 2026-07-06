#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

//初始化条件变量
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
//初始化互斥量
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define NUM_CONSUMER 5  //消费者线程数量
#define NUM_PRODUCER 2   //生产者线程数量

int balance; // 初始账户余额为0

// 消费者线程函数
void *consumer(void* arg) {
	int id = *((int*)arg);
	while (1) {
		int amount = rand() % 100; // 随机生成一个金额
		pthread_mutex_lock(&mutex);
		do {
			pthread_cond_wait(&cond, &mutex); //解锁，阻塞等待, 收到信号以后再加锁,访问临界资源
		} while(balance < amount);
		balance -= amount; // 消费
		printf("[消费者ID:%d]消费 %d 元: 当前余额为 %d 元\n", id, amount, balance);
		pthread_mutex_unlock(&mutex);
		sleep(rand() % 3); // 随机休眠
	}
	return NULL;
}

// 生产者线程
void* producer(void* arg) {
	int id = *((int*)arg);
	while (1) {
		int amount = rand() % 100; // 随机生成一个金额
		pthread_mutex_lock(&mutex);
		balance += amount; // 增加余额
		printf("[生产者ID:%d] 存入 %d 元，余额为 %d 元\n", id, amount, balance);
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
		sleep(rand() % 3); // 随机休眠
	}
	return NULL;
}

int main(int argc, const char *argv[])
{
	pthread_t ctid[NUM_CONSUMER];
	pthread_t ptid[NUM_PRODUCER];
	int cids[NUM_CONSUMER];
	int pids[NUM_PRODUCER];

	srand(time(NULL)); // 初始化随机数种子

	for (int i = 0; i < NUM_CONSUMER; i++) {
		cids[i] = i + 1;
		pthread_create(&ctid[i], NULL, consumer, &cids[i]);
	}

	for (int i = 0; i < NUM_PRODUCER; i++) {
		pids[i] = i + 1;
		pthread_create(&ptid[i], NULL, producer, &pids[i]);
	}

	for (int i = 0; i < NUM_CONSUMER; i++) {
		pthread_join(ctid[i], NULL);
	}

	for (int i = 0; i < NUM_PRODUCER; i++) {
		pthread_join(ptid[i], NULL);
	}

	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
	return 0;
}