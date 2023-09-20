# LrnWebServer

学习linux多线程服务器编程，实现的webserver。

## 实施计划

- [x] 使用epoll实现I/O复用
- [x] 实现线程池
- [x] 实现HTTP协议解析和逻辑处理
- [x] 使用定时器处理非活跃连接
- [ ] 实现日志系统
- [ ] 待定...

## 测试

```bash
curl 'http://localhost:1234/favicon.ico'   -H 'Accept: image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8'   -H 'Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6'   -H 'Connection: keep-alive'   -H 'DNT: 1'   -H 'Referer: http://localhost:1234/'   -H 'Sec-Fetch-Dest: image'   -H 'Sec-Fetch-Mode: no-cors'   -H 'Sec-Fetch-Site: same-origin'   -H 'User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36 Edg/116.0.1938.81'   -H 'sec-ch-ua: "Chromium";v="116", "Not)A;Brand";v="24", "Microsoft Edge";v="116"'   -H 'sec-ch-ua-mobile: ?0'   -H 'sec-ch-ua-platform: "Windows"'   --compressed --output favicon.ico

# 获取的index.html
# <!DOCTYPE html>
# <html>
# <head>
# <meta charset="utf-8">
# <title>LrnWebServer</title>
# </head>
# <body>

# <h1>Hello world!</h1>
# <p>Hello world!</p>

# </body>
```

获取直接使用浏览器访问`localhost:1234`。

![](/image/brower-test.png)

## 待解决

* HttpConnection对象的生命周期管理，目前是在WebServer类中实例化一个大数组，使用socketFd去索引，close掉socketFd之后，可复用。这样会不会浪费空间？毕竟`MAX_FD`有65535这么大。哈希表？

## 记录

* 设置SO_LINGER套接字选项之后，在调用`close()`时会将套接字缓存中的所有数据全部发送出去或者超时，需要根据`close()`的返回值判断。
* 设置SO_REUSEADDR套接字选项后，在关闭套接字之后无需等待TIME_WAIT（TIME_WAIT是TCP报文在网络中存在的最久的时间，防止新连接如果使用旧连接的端口会接收旧连接的数据）的时间，即可重新使用该端口。监听套接字要设置该选项的原因是重启服务器时需要立即重新复用端口，不设置该选项会导致`bind()`失效。
* 模板类的成员函数定义为建议放在头文件中。
* 线程的可结合和可分离，线程默认被设置为可结合，表示线程可以被其他线程或者进程杀死或者回收资源，比如在一个进程中创建线程，该线程必须有该进程控制生命周期。而如果被设置为可分离，则线程的生命周期与该进程无关。

## 压力测试

### webbench安装

```bash
wget http://home.tiscali.cz/~cz210552/distfiles/webbench-1.5.tar.gz
tar -zxvf webbench-1.5.tar.gz
cd webbench-1.5
make
sudo make install
```

### 出错处理

执行`make`出错。

```bash
webbench.c:21:10: fatal error: rpc/types.h: No such file or directory
   21 | #include <rpc/types.h>
      |          ^~~~~~~~~~~~~
compilation terminated.
make: *** [<builtin>: webbench.o] Error 1
```

解决步骤：

```bash
sudo apt-get install -y libtirpc-dev                        # 安装rpc库
sudo ln -s /usr/include/tirpc/rpc/types.h /usr/include/rpc  # 建立软连接 rpc/types.h
sudo ln -s /usr/include/tirpc/netconfig.h /usr/include      # 建立软连接 netconfig.h
sudo apt-get install universal-ctags                        # 安装ctags
```

> [在WSL中的ubuntu中安装webbench报错处理](https://beltxman.com/3874.html)

### 测试

```bash
webbench -c <clients> -t <time> <url>   # clients为并发连接数，time为持续时间
# 例
# webbench -c 5 -t 30 http://localhost:1234/
```