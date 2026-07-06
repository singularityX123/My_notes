# 从GET到POST

### GET的缺点

get的请求信息会直接展示在URL当中

### POST如何获取表单数据？

```
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
        printf("Content-Type: text/html\n\n");
        printf("<!DOCTYPE html>\n");
        printf("<html> <head> <meta charset=\"utf-8\">\n");
        printf("<title>我的html页面</title> </head> <body>\n");
        printf("<h1>请求方式：REQUEST_METHOD:%s</h1>\n", getenv("REQUEST_METHOD"));
        printf("<p>CONTENT_LENGTH:%s</p>\n", getenv("CONTENT_LENGTH"));
        printf("<p>CONTENT_TYPE:%s</p>\n", getenv("CONTENT_TYPE"));
        char buf[1024] = {};
        scanf("%s", buf);
        printf("<p>but:%s</p>\n", buf);
        printf("</body> </html> \n");
        return 0;
}

```

### 关于HTTP当中的内容类型

常见的媒体格式类型如下：

- text/html ： HTML格式
- text/plain ：纯文本格式
- text/xml ： XML格式
- image/gif ：gif图片格式
- image/jpeg ：jpg图片格式
- image/png：png图片格式

以application开头的媒体格式类型：

- application/xhtml+xml ：XHTML格式
- application/xml： XML数据格式
- application/atom+xml ：Atom XML聚合格式
- application/json： JSON数据格式
- application/pdf：pdf格式
- application/msword ： Word文档格式
- application/octet-stream ： 二进制流数据（如常见的文件下载）
- application/x-www-form-urlencoded ： <form encType="">中默认的encType，form表单数据被编码为key/value格式发送到服务器（表单默认的提交数据的格式）

另外一种常见的媒体格式是上传文件之时使用的：

- multipart/form-data ： 需要在表单中进行文件上传时，就需要使用该格式

### 最终代码：

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
        printf("Content-Type: text/html\n\n");
        printf("<!DOCTYPE html>\n");
        printf("<html> <head> <meta charset=\"utf-8\">\n");
        printf("<title>我的html页面</title> </head> <body>\n");
        if(strncasecmp("POST", getenv("REQUEST_METHOD"), 4) == 0 &&
                        atoi(getenv("CONTENT_LENGTH") ) > 0) {
                printf("<h1>请求方式是POST类型</h1>\n");
                printf("<p>%s</p>\n", getenv("CONTENT_LENGTH"));
        } else {
                printf("<h1>请求方式为空</h1>\n");
                return -1;
        }
        char *content_type = "application/x-www-form-urlencoded";
        if(strncasecmp(content_type, getenv("CONTENT_TYPE"), strlen(content_type) ) == 0) {
                char username[50], password[50];
                if( scanf("username=%49[^&]&password=%49s", username, password) != 2) {
                    printf("<h3>\"账号或密码错误！\"</h3>\n");
                }
                printf("<h1>欢迎【%s】登录本网站!</h1>", username);
        }
        printf("</body> </html> \n");
        return 0;
}
```

