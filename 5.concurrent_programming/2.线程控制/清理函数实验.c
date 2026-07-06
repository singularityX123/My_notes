#define a // 线程被取消时执行清理函数
//#define b // 线程调用 pthread_exit(3) 终止时执行清理函数  
//#define c // 显式调用 pthread_cleanup_pop() 且 execute 非零时
//#define d // 解决异步取消带来的问题


#ifdef a // 线程被取消时执行清理函数

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void routine1(void *arg) {
	printf("run routine1.\n");
}

void routine(void *arg) {
	printf("run routine.\n");
}

void* thread_func(void* arg) {
	printf("线程开始运行\n");

	pthread_cleanup_push(routine, NULL);
	pthread_cleanup_push(routine1, NULL);
	// 模拟长时间运行的操作，并在某些点处设置取消点
	for (int i = 0; i < 3; ++i) {
		printf("线程运行中：%d\n", i);
		sleep(1);
	}
	pthread_cleanup_pop(0);
	pthread_cleanup_pop(0);

	printf("线程正常结束\n");
	pthread_exit(NULL);
}

int main() {
	pthread_t tid;
	// 创建线程
	pthread_create(&tid, NULL, thread_func, NULL);
	sleep(1);
	pthread_cancel(tid);

	// 等待线程结束
	pthread_join(tid, NULL);
	printf("主线程结束\n");
	return 0;
}

#elif defined(b) // 线程调用 pthread_exit(3) 终止时执行清理函数

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void routine1(void *arg) {
	printf("run routine1.\n");
}

void routine(void *arg) {
	printf("run routine.\n");
}

void* thread_func(void* arg) {
	printf("线程开始运行\n");

	pthread_cleanup_push(routine, NULL);
	pthread_cleanup_push(routine1, NULL);
	// 模拟长时间运行的操作，并在某些点处设置取消点
	pthread_exit(NULL); //线程调用 pthread_exit(3) 终止时, 清理函数会被执行
	// return NULL;  //如果是直接返回，取消函数将不会被执
	pthread_cleanup_pop(0);
	pthread_cleanup_pop(0);

	printf("线程正常结束\n");
	pthread_exit(NULL);
}

int main() {
	pthread_t tid;
	// 创建线程
	pthread_create(&tid, NULL, thread_func, NULL);
	sleep(1);
	pthread_cancel(tid);

	// 等待线程结束
	pthread_join(tid, NULL);
	printf("主线程结束\n");
	return 0;
}

#elif defined(c) // 显式调用 pthread_cleanup_pop() 且 execute 非零时

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void routine1(void *arg) {
	printf("run routine1.\n");
}

void routine(void *arg) {
	printf("run routine.\n");
}

void* thread_func(void* arg) {
	printf("线程开始运行\n");

	pthread_cleanup_push(routine, NULL);
	pthread_cleanup_push(routine1, NULL);
	for (int i = 0; i < 3; ++i) {
		printf("线程运行中：%d\n", i);
		sleep(1);
	}
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);

	printf("线程正常结束\n");
	pthread_exit(NULL);
}

int main() {
	pthread_t tid;
	// 创建线程
	pthread_create(&tid, NULL, thread_func, NULL);
	sleep(1);
	pthread_cancel(tid);

	// 等待线程结束
	pthread_join(tid, NULL);
	printf("主线程结束\n");
	return 0;
}

#elif defined(d) // 解决异步取消带来的问题

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int glob;  //临界资源
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 静态初始化互斥量

void CleanupHandle(void *arg) {
	pthread_mutex_unlock(&mutex);
}

void *ThreadFunc(void *arg) {
	int loops = *((int *)arg);
	int loc, j;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	for(j = 0; j < loops; j++) {
		pthread_mutex_lock(&mutex);
		pthread_cleanup_push(CleanupHandle, NULL);
		glob++;
		sleep(4);
		pthread_cleanup_pop(1);
	}

	pthread_exit(NULL);
}

int main(int argc, const char *argv[])
{
	pthread_t tid1, tid2;
	int loops;

	if(argc != 2) {
		fprintf(stderr, "%s [loops]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if( sscanf(argv[1], "%d", &loops) != 1) {
		fprintf(stderr, "Invalid loops\n");
		exit(EXIT_FAILURE);
	}

	int ret = pthread_create(&tid1, NULL, ThreadFunc, &loops);
	if(ret != 0) {
		fprintf(stderr, "pthread_create:%s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}
	ret = pthread_create(&tid2, NULL, ThreadFunc, &loops);
	if(ret != 0) {
		fprintf(stderr, "pthread_create:%s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}

	usleep(10000);
	pthread_cancel(tid1);
	void *retval;
	ret = pthread_join(tid1, &retval); 
	if(ret != 0) {
		fprintf(stderr, "pthread_join tid1:%s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}
	if(PTHREAD_CANCELED == (int)retval) {
		printf("线程1被取消\n");
	} else {
		printf("线程1被正常终止\n");
	}

	ret = pthread_join(tid2, &retval);
	if(ret != 0) {
		fprintf(stderr, "pthread_join tid2:%s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}
	if(PTHREAD_CANCELED == (int)retval) {
		printf("线程2被取消\n");
	} else {
		printf("线程2被正常终止\n");
	}


	printf("glob = %d\n", glob);
	return 0;
}

#endif