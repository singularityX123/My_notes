#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int glob;  //临界资源
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 静态初始化互斥量

void *ThreadFunc(void *arg) {
	int loops = *((int *)arg);
	int loc, j;

	for(j = 0; j < loops; j++) {
		pthread_mutex_lock(&mutex);
		//加锁与解锁的代码区域被称为 临界区
		loc = glob;
		loc++;
		glob = loc;
		pthread_mutex_unlock(&mutex);
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

	ret = pthread_join(tid1, NULL); 
	if(ret != 0) {
		fprintf(stderr, "pthread_join tid1:%s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}

	ret = pthread_join(tid2, NULL);
	if(ret != 0) {
		fprintf(stderr, "pthread_join tid2:%s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}

	printf("glob = %d\n", glob);

	pthread_mutex_destroy(&mutex); // 销毁互斥量，释放相关资源
	
	return 0;
}