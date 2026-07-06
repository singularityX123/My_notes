#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

struct student{
	char name[32];
	int age;
	float score;
}*p;

int main(int argc, const char *argv[])
{
	int fd = open("stu.bin", O_RDWR);
	if(fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	// 映射共享内存
	p = mmap(NULL, sizeof(struct student), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(p == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	
	float score;
	printf("分数：%3.2f\n", p->score);
	while(1) {
		score = p->score;
		sleep(1);
		if(score != p->score)
			printf("分数变更为：%3.2f\n", p->score);
	}

	munmap(p, sizeof(struct student) );
	return 0;
}