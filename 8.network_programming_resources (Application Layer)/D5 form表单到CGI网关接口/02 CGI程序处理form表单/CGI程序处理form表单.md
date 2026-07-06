# CGI程序处理form表单

### 引入

#### 思考：怎样完成用户登录的功能？

1. `html`只是静态页面，无法动态的处理这种问题。需要依靠其它具备逻辑控制能力的语言来完这项工作
2. 处理用户登录功能的语言需要获取必要的数据

### 认识form表单

```html
    <form action="/cgi-bin/login.cgi" method="get">
          <input type="text" name="username" placeholder="请输入用户名"><br>
          <input type="password" name="password" placeholder="请输入密码"><br>
          <input type="submit" value="Login">
    </form>
```

- **`action="/cgi-bin/login.cgi"`**: 这个属性指定了当用户提交表单时，表单数据将被发送到服务器上的 `/cgi-bin/login.cgi` 文件进行处理。

- **`method="get"`**: 这个属性指定了表单数据的提交方式。

- **`type="submit"`**: 定义了一个提交按钮，当用户点击这个按钮时，表单数据将被发送到服务器。

### `CGI`程序处理form表单

```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
	char *data = getenv("QUERY_STRING");
	char username[50], password[50];

    //%49[^&]:表示从当前位置读取最多 49 个字符，直到遇到 &
	sscanf(data, "username=%49[^&]&password=%49s", username, password);

    //由于告诉浏览器，将发送html的文本页面
	printf("Content-Type: text/html\n\n");

    //登录成功以后的显示内容
	printf("<h1>Login successful! Welcome, %s!</h1>", username);

	return 0;
}
```

运行前需要让http服务器支持CGI功能：

```bash
sudo a2enmod cgi
sudo systemctl restart apache2
```

编译

```bash
gcc login.c -o login.cgi -Wall
```

把编写好的应用程序复制到`/usr/lib/cgi-bin/`目录下：

```bash
sudo cp login.cgi /usr/lib/cgi-bin/
```

打开浏览器进行测试

#### 思考：如果把`login.cgi`改成`login`程序还能正常运行吗？

`.cgi`是为了表示这是个`CGI`程序，和其它可执行程序是一样的，没有特殊的属性。

去掉`.cgi`的后缀不影响其功能，但文件名需要和`form`表单当中的文件名一致。例如：

1. 把`form`表单当中的`/cgi-bin/login.cgi`改为`/cgi-bin/login`，
2. 把`/usr/lib/cgi-bin/`下的`login.cgi` 改为`login` ，

再次进行测试...

