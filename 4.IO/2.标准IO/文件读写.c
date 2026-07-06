#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#define IO_char
//#define IO_line
//#define IO_binary
//#define IO_object
#define IO_format

int main(int argc, const char *argv[]){

    #ifdef IO_char
        FILE *fp = NULL;
        int c;
        int i;

        #if 0 // 单字符读
            
            if((fp = fopen("IO/标准IO/resources for practice/buffered.txt", "r+")) == NULL)
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

            if((fp = fopen("IO/标准IO/resources for practice/buffered.txt", "w+")) == NULL)
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
        
        #endif
        
    #elif defined(IO_line)
        FILE *fp = NULL;
        char buf[32] = {0}; // 初始化缓冲区为0

        #if 0 // 按行读

            if((fp = fopen("IO/标准IO/resources for practice/buffered.txt", "r+")) == NULL)
            {
                perror("fopen");
                return -1;
            }
            printf("文件打开成功！\n");

            while(1){
                if(fgets(buf, sizeof(buf), fp) == NULL) // 使用fgets()函数读取一行，输入的数据超出sizeof-1，sizeof-1个字符会保存到缓冲区，缓冲区最后添加一个'\0
                {
                    perror("fgets"); // 读取到文件末尾或出错时返回NULL
                    break;
                }
                printf("test look:%s ", buf);
            }
            printf("\n文件读取结束！\n");

            fclose(fp);
            fp = NULL;

        #endif

        #if 0 // 按行写

            if((fp = fopen("IO/标准IO/resources for practice/buffered.txt", "a+")) == NULL)
            {
                perror("fopen");
                return -1;
            }
            printf("文件打开成功！\n");

            for(int i = 0; i < 5; i++)
            {
                snprintf(buf, sizeof(buf), "This is line %d\n", i + 1); // 格式化字符串写入buf缓冲区
                fputs(buf, fp); // 使用fputs()函数写一行
            }

            fclose(fp);
            fp = NULL;

        #endif

    #elif defined(IO_binary)
        FILE *fp = NULL;
        char buf[32] = {0}; 

        # if 1 // 二进制块写
        if ((fp = fopen("binary.dat", "wb+")) == NULL) {
            perror("fopen");
            return -1;
        }
        printf("文件打开成功！\n");

        // 写入二进制数据
        char data[] = "Hello, World!";
        size_t ret = fwrite(data, sizeof(char), strlen(data), fp);
        if (ret != strlen(data)) {
            perror("fwrite");
            return -1;
        }
        printf("成功写入 %zu 字节数据。\n", ret);

        fclose(fp);
        fp = NULL;
        #endif

        # if 0 // 二进制块读
        if ((fp = fopen("binary.dat", "rb+")) == NULL) {
            perror("fopen");
            return -1;
        }
        printf("文件打开成功！\n");

        // 读取二进制数据
        ret = fread(buf, sizeof(char), strlen(data), fp);
        if (ret != strlen(data)) {
            perror("fread");
            return -1;
        }
        printf("成功读取 %zu 字节数据。\n", ret);
        printf("读取到的数据为: %s\n", buf);

        fclose(fp);
        fp = NULL;
        #endif


    #elif defined(IO_object) // 对象读写 基于二进制读写
        typedef struct STUDENT {
            int id;
            char name[20];
            float score;
        } Student;

        FILE *fp = NULL;
        Student stu_list[3] = {
            {1, "Alice", 85.5},
            {2, "Bob", 90.0},
            {3, "Charlie", 59.0}
        };

        size_t stu_size = sizeof(Student);

        // 写入数据
        if ((fp = fopen("IO/标准IO/resources for practice/student.dat", "wb")) == NULL) {
            perror("fopen for writing");
            return -1;
        }

        for (int i = 0; i < 3; i++) {
            if (fwrite(&stu_list[i], stu_size, 1, fp) != 1) {
                perror("fwrite");
                fclose(fp);
                return -1;
            }
            printf("已写入: %s\n", stu_list[i].name);
        }

        fclose(fp);

        // 读取数据
        if ((fp = fopen("IO/标准IO/resources for practice/student.dat", "rb")) == NULL) {
            perror("fopen for reading");
            return -1;
        }

        printf("\n读取学生信息:\n");
        Student stu_read;
        int count = 0;
        while (fread(&stu_read, stu_size, 1, fp) == 1) {
            printf("学生%d: ID=%d, Name=%s, Score=%.2f\n", 
                ++count, stu_read.id, stu_read.name, stu_read.score);
        }

        fclose(fp);
        fp = NULL;

    #elif defined(IO_format)

        #if 0 //使用流的格式化输入输出
            FILE *fp = NULL;
            if ((fp = fopen(argv[1], "a+")) == NULL) {
                perror("fopen");
                return -1;
            }

            // 格式化写入
            fprintf(fp, "%s", "Hello, formatted file I/O! Test fscanf next.\n");

            
            // 格式化读取
            char buffer[256];
            fscanf(stdin, "%s", buffer); 
            fprintf(fp, "%s", buffer);

            fclose(fp);
            fp = NULL;
        #endif

        #if 1 // 不使用流(使用字符串)的格式化输入输出
            int a, b, c;
            char buffer[256];
            // 验证当输入数据少于格式字符串要求时会发生什么
            sscanf("1 2", "%d %d %d", &a, &b, &c); // c未被赋值，保持未初始化状态; sscanf返回成功赋值的个数2
            sprintf(buffer, "a=%d, b=%d, c=%d\n", a, b, c);
            printf("%s", buffer);// 给人看的终端显示验证
        #endif

    #endif



    /* 实践一: 字符读写复制文件 */
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

    /* 实践二: 统计文本行 */
        // FILE *fp = NULL;
        // char buf[256] = {0};
        // int count = 0;

        // if ((fp = fopen("IO/标准IO/resources for practice/buffered.txt", "r")) == NULL) {
        //     perror("fopen");
        //     return -1;
        // }

        // #if 0 // 统计所有行（包括空行）
        // while (fgets(buf, sizeof(buf), fp) != NULL) {
        //     // 只要fgets成功读取（即使读取的是空行），就算一行
        //     // fgets只有在遇到EOF或出错时才返回NULL
        //     count++;
        // }
        // #endif
        
        
        // #if 1 // 排除空行的统计（替换上面的while循环）
        // while (fgets(buf, sizeof(buf), fp) != NULL) {
        //     // 排除完全空的行（只包含换行符）
        //     if (buf[0] != '\n') {
        //         count++;
        //     }
        // }
        // #endif

        // printf("文件共有 %d 行\n", count);

        // fclose(fp);
        // fp = NULL;
    /****************************************/

    /* 实践三: 字符读写复制文件 */
        // FILE *fp = NULL;
        // char buf[256] = {0};
        
        // if ((fp = fopen("resources for practice/original.txt", "r")) == NULL) {
        //     perror("fopen");
        //     return -1;
        // }

        // FILE *fp_copy = NULL;
        // if ((fp_copy = fopen("resources for practice/goal.txt", "a+")) == NULL) {
        //     perror("fopen");
        //     return -1;
        // } 

        // while (1){
        //     if(fgets(buf, sizeof(buf), fp) == NULL) // 使用fgets()函数读取一行
        //         {
        //             break;
        //         }
        //     fputs(buf, fp_copy); // 使用fputs()函数写一行
        // }
        
        // fflush(fp_copy); // 刷新缓冲区，确保所有数据都写入文件
        // printf("文件复制成功！\n");

        // fclose(fp_copy);
        // fp_copy = NULL; 
        // fclose(fp);
        // fp = NULL;
    /****************************************/

    /* 实践四: 二进制读写复制文件 */
        // FILE *fp_r = NULL, *fp_w = NULL;
        // char buf[1024] = {0};
        // int error = 0;
        // size_t bytes_read = 0, total_bytes = 0;
        
        // if (argc < 3) {
        //     printf("Usage: %s <source file> <destination file>\n", argv[0]);
        //     return -1;
        // }

        // // 打开源文件
        // if ((fp_r = fopen(argv[1], "rb")) == NULL) {
        //     perror("fopen source file");
        //     return -1;
        // }
        
        // // 打开目标文件
        // if ((fp_w = fopen(argv[2], "wb")) == NULL) {
        //     perror("fopen destination file");
        //     fclose(fp_r);  // 关闭已打开的源文件
        //     return -1;
        // }

        // // 复制文件
        // while ((bytes_read = fread(buf, sizeof(char), sizeof(buf), fp_r)) > 0) {
        //     size_t bytes_written = fwrite(buf, sizeof(char), bytes_read, fp_w);
            
        //     if (bytes_written != bytes_read) {
        //         perror("fwrite failed");
        //         error = 1;
        //         break;
        //     }
        //     total_bytes += bytes_written;
        // }

        // // 检查读取错误
        // if (ferror(fp_r)) {
        //     perror("fread failed");
        //     error = 1;
        // }

        // // 关闭文件
        // if (fp_r) {
        //     fclose(fp_r);
        //     fp_r = NULL;
        // }
        // if (fp_w) {
        //     fclose(fp_w);
        //     fp_w = NULL;
        // }

        // // 输出结果
        // if (!error) {
        //     printf("file copied successfully from %s to %s (%zu bytes)\n", 
        //         argv[1], argv[2], total_bytes);
        //     return 0;
        // } else {
        //     return -1;
        // }   
    /****************************************/

        return 0;
    }