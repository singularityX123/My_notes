#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
int main(void) {
// 打开或创建文件1.txt ，以追加写入模式（O_APPEND）
    int fd = open("1.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (fd == -1) {
        perror("open failed"); exit(1); 
    }

    int line_number = 1;
    char time_buffer[64];
    char line_buffer[128];

    while (1) {
        // 获取当前时间并格式化
        time_t raw_time;
        struct tm *time_info;
        time(&raw_time);
        time_info = localtime(&raw_time);
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);

        // 构造带行号的字符串
        int len = snprintf(line_buffer, sizeof(line_buffer), "%d. %s\n", line_number, time_buffer);   

        // 写入文件
        ssize_t bytes_written = write(fd, line_buffer, len);

        if (bytes_written == -1) {
            perror("write failed");
            close(fd);
            exit(1);
        }

        line_number++;

        sleep(1);  // 等待1秒,防止文件过大CPU占用过高
    }

    close(fd);

    return 0;
}