#include "webserver.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>
#include <strings.h>
#include <iostream>

const int MAX_EVENT_NUMBER = 10000;
const int BUFFER_SIZE = 1024;

int WebServer::setNonblocking(int fd)
{
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

void WebServer::addFd(int epollFd, int fd, bool enableET)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if (enableET)
    {
        event.events |= EPOLLET;
    }
    epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    setNonblocking(fd);
}

void WebServer::doWithRead(int sockFd)
{
    char buf[BUFFER_SIZE];
    // ET模式
    if (m_epollTriggerMode == true)
    {
        // ET模式下可读事件只会触发一次，所以需要循环读，把缓存中的数据全部读出
        while (1)
        {
            memset(buf, '\0', BUFFER_SIZE);
            int ret = recv(sockFd, buf, BUFFER_SIZE - 1, 0);
            if (ret < 0)
            {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                {
                    std::cout << "doWithRead(): 待会读。" << std::endl;
                    break;
                }
                close(sockFd);
            }
            else if (ret == 0)
            {
                close(sockFd);
            }
            else
            {
                std::cout << "doWithRead(): 读取" << ret << "字节的内容。" << std::endl;
            }
        }
    }
    else // LT模式
    {
        memset(buf, '\0', BUFFER_SIZE);
        int ret = recv(sockFd, buf, BUFFER_SIZE - 1, 0);
        if (ret <= 0)
        {
            close(sockFd);
            return ;
        }
        std::cout << "doWithRead(): 读取" << ret << "字节的内容。" << std::endl;
    }
}

WebServer::WebServer(int port) : 
    m_listenFd(-1),
    m_port(port), 
    m_epollFd(-1),
    m_epollTriggerMode(true)
{

}

WebServer::~WebServer()
{
}

void WebServer::init()
{
    // 创建监听套接字
    m_listenFd = socket(PF_INET, SOCK_STREAM, 0);
    if (m_listenFd < 0)
    {
        std::cerr << "socket(): 创建监听套接字失败。" << std::endl; 
        return ;
    }
    // 设置SO_LINGER选项
    struct linger lingerSetting;
    lingerSetting.l_onoff = 1;
    lingerSetting.l_linger = 1; // 秒
    setsockopt(m_listenFd, SOL_SOCKET, SO_LINGER, &lingerSetting, sizeof(lingerSetting));

    struct sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(m_port);

    int flag = 1;
    // 设置SO_REUSERADDR
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    int ret = bind(m_listenFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0)
    {
        std::cerr << "bind(): 绑定套接字失败。" << std::endl;
        return ;
    }
    ret = listen(m_listenFd, 5); // 第二个参数定义等待连接队列的最大容量
    if (ret < 0)
    {
        std::cerr << "listen(): 监听失败。" << std::endl;
        return ;
    }
    // 创建epoll内核事件表
    m_epollFd = epoll_create(5); // 参数给内核一个提示，告诉事件表需要多大，现在并不起作用
    if (m_epollFd == -1)
    {
        std::cerr << "epoll_create(): 执行失败。" << std::endl;
        return ;
    }
    // 注册epoll事件
    addFd(m_epollFd, m_listenFd, m_epollTriggerMode);
}

void WebServer::eventLoop()
{
    epoll_event events[MAX_EVENT_NUMBER];
    while (1)
    {
        int num = epoll_wait(m_epollFd, events, MAX_EVENT_NUMBER, -1); // -1表示永远阻塞不会超时
        if (num < 0)
        {
            std::cerr << "epoll_wait(): 执行出错。" << std::endl;
            break;
        }
        for (int i = 0; i < num; i++)
        {
            int sockFd = events[i].data.fd;
            if (sockFd == m_listenFd)
            {
                struct sockaddr_in clientAddr;
                socklen_t clientAddrLength = sizeof(clientAddr);
                int connFd = accept(m_listenFd, (struct sockaddr *)&clientAddr, &clientAddrLength);
                addFd(m_epollFd, connFd, m_epollTriggerMode);
            }
            else if (events[i].events & EPOLLIN)
            {
                doWithRead(sockFd);
            }
            else
            {
                // 暂不处理
            }
        }
    }
}
