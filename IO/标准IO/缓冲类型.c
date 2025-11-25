#include <stdio.h>
#include <unistd.h>
//#define __unbuffered__  //无缓冲
//#define __line_buffered__ //行缓冲
//#define __fully_buffered__ //全缓冲

#define __change_buffer_type__ //改变缓存类型

int main(int argc, char *argv[])
{
#ifdef __unbuffered__

    perror("unbuffered"); //标准错误输出是无缓冲的
    #if 1 //设置等待，验证perror的输出真的是无缓冲的
        while (1){
            //sleep(1);
            usleep(10000); //如果是有缓冲的，程序未结束前/未刷新缓冲区时/缓存区溢出时是看不到输出的

            /*  
            *  <能看到输出的情况（会看到一大串无间隔的"a"）>
            *   
            * 缓冲区满时：标准输出缓冲区通常是4KB（4096字节）
            * 程序结束时：所有缓冲区都会被自动刷新
            * 手动刷新：使用fflush(stdout)*/
            printf("a"); 
            
        }
    #endif

#elif defined(__line_buffered__)

        do {
           printf("line buffered\n");//有换行符时强制刷新缓冲区
        } 
        while (1);
        {
            sleep(1);
        } // ？我这写得何意味
        

#elif defined(__fully_buffered__)

        FILE *fp = fopen("buffered.txt", "a+"); //a (append): 追加模式，写入时从文件末尾开始; +: 可读写模式
        
        fprintf(fp, "fully buffered"); //无换行符，不会刷新缓冲区

        printf("%lu ", fp-> _IO_buf_end - fp-> _IO_buf_base); //全缓冲的缓冲区大小通常为4096字节，这与计算机的分页机制有关。
        int count = 0;
        do{
            fprintf(fp, "a"); //写入缓冲区
            count++;
        }while (count < 4095);

        printf("%lu ", fp-> _IO_write_ptr - fp-> _IO_write_base); //缓冲区已满，写入4096字节后，缓冲区会被刷新，文件中会有内容
        sleep(5); //等待5秒，验证缓冲区刷新情况

#elif defined(__change_buffer_type__)

        printf("a"); 
        sleep(5);

        setbuf(stdout, NULL); //将标准输出设置为无缓冲，a立即输出
        printf("b"); //无缓冲，b又立即输出
        sleep(5);

#endif

    return 0;
}