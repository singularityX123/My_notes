## 安装Web服务器

1. **更新系统**  
在安装任何软件之前，先确保系统是最新的：

```plain
sudo apt update
sudo apt upgrade
```

2. **安装Apache Web服务器**  
Apache是一个流行的开源Web服务器。你可以通过以下命令安装它：

```plain
sudo apt install apache2
```

安装完成后，Apache将自动启动。可以通过访问`http://localhost`查看默认的Apache欢迎页面。

3. **检查Apache状态**  
你可以使用以下命令检查Apache是否正在运行：

```plain
sudo systemctl status apache2
```

4. **测试服务器**  
打开浏览器，输入`http://<your-server-ip-address>`，如果看到Apache默认页面，说明服务器搭建成功。

5. **重启Apache**  
在更改配置后，需要重启Apache服务器：

```plain
sudo systemctl restart apache2
```

## 如何使用
配置文件：`/etc/apache2`

html页面文件：`/var/www/html/`

