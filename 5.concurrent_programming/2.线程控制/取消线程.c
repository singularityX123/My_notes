#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void* thread_func(void* arg) {
	printf("线程开始运行\n");

	// 模拟长时间运行的操作，并在某些点处设置取消点
	for (int i = 0; i < 10; ++i) {
		printf("线程运行中：%d\n", i);
		sleep(1);
	}

	printf("线程正常结束\n");
	pthread_exit(NULL);
}

int main() {
	pthread_t tid;
	// 创建线程
	pthread_create(&tid, NULL, thread_func, NULL);
	sleep(3);
    // TODO 取消线程关键代码
	pthread_cancel(tid);

	// 等待线程结束
	pthread_join(tid, NULL);
	printf("主线程结束\n");
	return 0;
}