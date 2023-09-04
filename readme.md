# LrnWebServer

学习linux多线程服务器编程，实现的webserver。

## 测试

目前使用epoll实现了I/O复用，可以使用telnet验证程序的正确性。 
步骤：
* 使用cmake编译并运行服务端程序
* 使用telnet连接: `telnet localhost 1234`

```bash
# 输入
# Trying 127.0.0.1...
# Connected to localhost.
# Escape character is '^]'.
# hello
# hi
# ^]
# 输出
# doWithRead(): 读取7字节的内容。
# doWithRead(): 待会读。
# doWithRead(): 读取5字节的内容。
# doWithRead(): 待会读。
# doWithRead(): 读取6字节的内容。
# doWithRead(): 待会读。
```

## 记录

* 设置SO_LINGER套接字选项之后，在调用`close()`时会将套接字缓存中的所有数据全部发送出去或者超时，需要根据`close()`的返回值判断。
* 设置SO_REUSEADDR套接字选项后，在关闭套接字之后无需等待TIME_WAIT（TIME_WAIT是TCP报文在网络中存在的最久的时间，防止新连接如果使用旧连接的端口会接收旧连接的数据）的时间，即可重新使用该端口。监听套接字要设置该选项的原因是重启服务器时需要立即重新复用端口，不设置该选项会导致`bind()`失效。