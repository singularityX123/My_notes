/* 直观比较多线程下，运算速度明显要快很多。*/

#if 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, const char *argv[])
{
	unsigned long long a;
	if(argc != 2) {
		fprintf(stderr, "Usage %s [loops]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if(sscanf(argv[1], "%llu", &a) != 1) {
		fprintf(stderr, "loops Invalid.\n");
		exit(EXIT_FAILURE);
	}

	time_t t1;
	time(&t1);

	for(unsigned long long i = 0; i < a; i++);

	time_t t2;
	time(&t2);
	printf("进程使用了 %ld 秒\n", t2 - t1);
	return 0;
}

#endif

#if 0 // 使用多线程再次实现（4个线程,代码必须在具有4线程以上的CPU当中运行。否则，体现不出任何意义）

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

void *ThreadFunc(void *arg) {
	unsigned long long a = *(unsigned long long *)arg;
	for(unsigned long long i = 0; i < a; i++);
	pthread_exit(NULL);
}

int main(int argc, const char *argv[])
{
	unsigned long long a;
	if(argc != 2) {
		fprintf(stderr, "Usage %s [loops]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if(sscanf(argv[1], "%llu", &a) != 1) {
		fprintf(stderr, "loops Invalid.\n");
		exit(EXIT_FAILURE);
	}

	time_t t1;
	time(&t1);

	unsigned long long loops = a / 4;
	pthread_t tid[4] = {};
	for(int i = 0; i < 4; i++) {
		int ret = pthread_create(&tid[i], NULL, ThreadFunc, &loops);
		if(ret != 0) {
			fprintf(stderr, "pthread_create:%s\n", strerror(ret) );
			exit(EXIT_FAILURE);
		}
	}

	for(int i = 0; i < 4; i++) {
		int ret = pthread_join(tid[i], NULL);
		if(ret != 0) {
			fprintf(stderr, "pthread_join:%s\n", strerror(ret) );
			exit(EXIT_FAILURE);
		}
	}

	time_t t2;
	time(&t2);
	printf("进程使用了 %ld 秒\n", t2 - t1);
	return 0;
}

#endif
