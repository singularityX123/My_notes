#include <stdio.h>
//#define IO_char
#define IO_line
//#define IO_object

int main(int argc, const char *argv[])
{

#ifdef IO_char{
    FILE *fp = NULL;
    int c;
    int i;

    #if 0 // 单字符读
        
        if((fp = fopen("buffered.txt", "r+")) == NULL)
        {
            perror("fopen");
            return -1;
        }
        printf("文件打开成功！\n");


        while(1){
            //if((c = fgetc(fp)) == EOF) // 使用fgetc()函数读取字符
            if((c = getc(fp)) == EOF) // 使用getc()作为宏实现
            {
                break;
            }
            printf("%c", (char)c);
        }

        fclose(fp);
        fp = NULL;
    #endif

    #if 0 // 单字符写

        if((fp = fopen("buffered.txt", "w+")) == NULL)
        {
            perror("fopen");
            return -1;
        }
        printf("文件打开成功！\n");

        for(i = 0; i < 26; i++)
        {
            fputc('A' + i, fp); // 使用fputc()函数写字符
            //putc('A' + i, fp); // 使用putc()作为宏实现
        }

        fclose(fp);
        fp = NULL;
    
    #endif}
    
#elif defined IO_line

#endif



/* 实践: 字符读写复制文件 */
    // FILE *fp = NULL;
    // int c;
    // fp = fopen("resources for practice/original.txt", "r"); // win11和linux路径不一样,这是linux路径
    // if (fp == NULL){
    //     perror("fopen");
    //     return -1;
    // }

    // FILE *fp_copy = NULL;
    // fp_copy = fopen("resources for practice/goal.txt", "w+");
    // if (fp_copy == NULL){
    //     perror("fopen");
    //     return -1;
    // }
    // while (1){
    //     if ((c = fgetc(fp)) == EOF){
    //         break;
    //     }
    //     fputc(c, fp_copy);
    // }
    // printf("文件复制成功！\n");
    
    // fclose(fp);
    // fp = NULL;
    // fclose(fp_copy);
    // fp_copy = NULL;
/****************************************/

    return 0;
}