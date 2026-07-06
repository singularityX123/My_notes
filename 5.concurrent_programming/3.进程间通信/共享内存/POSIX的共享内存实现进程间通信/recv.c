#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdlib.h>
#include <unistd.h>
int main(int argc, const char *argv[])
{
	// shm_open() 创建并打开一个新对象
	int fd = shm_open("/object", O_RDONLY, 0666);
	if(fd == -1) {
		perror("shm_open");
		exit(EXIT_FAILURE);
	}
#if 0
	// ftruncate() 设置共享内存对象的大小。
	ftruncate(fd, 4096);
#endif
	// mmap() 将共享内存对象映射到调用进程的虚拟地址空间。
	char *str = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
	if(str == MAP_FAILED) {
		perror("shm_open");
		exit(EXIT_FAILURE);
	}
	// 关闭共享内存对象
	close(fd);

	printf("%s", str);

	// munmap() 从调用进程的虚拟地址空间取消共享内存对象的映射。
	munmap(str, 4096);
	// shm_unlink() 删除共享内存对象名称。
	shm_unlink("/object");

	return 0;
}