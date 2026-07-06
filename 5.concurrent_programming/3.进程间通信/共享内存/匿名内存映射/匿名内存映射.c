#define MAP_ANONYMOUS	0x20

/*  #define MAP_SHARED     0x01    // 共享映射
    #define MAP_PRIVATE    0x02    // 私有映射
    #define MAP_FIXED      0x10    // 固定地址映射
    #define MAP_ANONYMOUS  0x20    // 匿名映射     */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    // 创建匿名共享内存
    size_t size = 4096;
    char *shm_ptr = mmap(NULL, size, 
                         PROT_READ | PROT_WRITE, 
                         MAP_SHARED | MAP_ANONYMOUS, 
                         -1, 0);
    
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    // 父进程写入数据
    const char *message = "Hello from parent process";
    strncpy(shm_ptr, message, size - 1);
    shm_ptr[size - 1] = '\0';  // 确保字符串终止

    printf("Parent wrote: %s\n", shm_ptr);

    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        munmap(shm_ptr, size);
        return 1;
    }
    
    if (pid == 0) {
        // 子进程
        printf("Child process read: %s\n", shm_ptr);
        
        // 子进程也可以修改数据
        strncpy(shm_ptr, "Modified by child", size - 1);
        shm_ptr[size - 1] = '\0';
        
        munmap(shm_ptr, size);
        exit(0);
    } else {
        // 父进程
        wait(NULL);  // 等待子进程结束
        
        printf("Parent sees after child: %s\n", shm_ptr);
        
        munmap(shm_ptr, size);
    }

    return 0;
}