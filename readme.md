# LrnWebServer

学习linux多线程服务器编程，实现的webserver。

## 实施计划

- [x] 使用epoll实现I/O复用
- [x] 实现线程池
- [x] 实现HTTP协议解析和逻辑处理
- [ ] 使用定时器
- [ ] 实现日志系统
- [ ] 待定...

## 测试

```bash
curl 'http://localhost:1234/' \
  -H 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7' \
  -H 'Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6' \
  -H 'Cache-Control: max-age=0' \
  -H 'Connection: keep-alive' \
  -H 'DNT: 1' \
  -H 'Sec-Fetch-Dest: document' \
  -H 'Sec-Fetch-Mode: navigate' \
  -H 'Sec-Fetch-Site: none' \
  -H 'Sec-Fetch-User: ?1' \
  -H 'Upgrade-Insecure-Requests: 1' \
  -H 'User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36 Edg/116.0.1938.76' \
  -H 'sec-ch-ua: "Chromium";v="116", "Not)A;Brand";v="24", "Microsoft Edge";v="116"' \
  -H 'sec-ch-ua-mobile: ?0' \
  -H 'sec-ch-ua-platform: "Windows"' \
  --compressed ;

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