// 改善后仍然存在严重的竞态条件问题，需要使用信号量来解决

/*process1.c*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define SHM_SIZE 1024  // 共享内存大小

int main() {
    key_t key = ftok(".", 65);  // 创建一个唯一的键值
    if (key == -1) {
        perror("ftok failed");
        exit(1);
    }

    // 创建共享内存
    int shm_id = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);  // 0666: 共享内存的权限
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    // 将共享内存附加到当前进程的地址空间
	int *data = shmat(shm_id, NULL, 0);  // shmat返回指向共享内存的指针
    if (data == (int *) -1) {
        perror("shmat failed");
        exit(1);
    }
	*data = 10;
	while(*data != 0);
	sleep(1);
	for(int i = 0; i < 100; i++) (*data)++;
	sleep(1);
	printf("-------\n");
	printf("data = %d\n", *data);
    // 断开与共享内存的连接
    if (shmdt(data) == -1) {
        perror("shmdt failed");
        exit(1);
    }

    return 0;
}