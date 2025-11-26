#include <stdio.h>
#include <string.h>
#include <errno.h>
int main(int argc, char * argv[]) {
 if(argc < 2){
  printf("Usage: %s <filename>\n", argv[0]);
  return -1;
 }
 const char * filename = argv[1];
 // 尝试打开文件
 FILE * fp = fopen(filename, "r");
 if (fp == NULL) {
  // 使用 perror 输出错误信息
  perror(filename);
  // 获取 errno 对应的错误描述字符串
  const char *errmsg = strerror(errno);
  // 手动格式化并输出错误信息
  fprintf(stderr, "%s %s\n", filename, errmsg);
 } else {
  fprintf(stdout,"%s Open success\n", filename);
  // 文件打开成功，关闭文件
  fclose(fp);
 }
 return 0;
}