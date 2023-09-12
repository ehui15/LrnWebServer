#include "webserver.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>
#include <strings.h>
#include <iostream>

#include "httpconnection.h"

const int MAX_EVENT_NUMBER = 10000;
const int BUFFER_SIZE = 1024;
const int MAX_FD = 65535; // todo

int WebServer::setNonblocking(int fd)
{
    int oldOption = fcntl(fd, F_GETFL);
    int newOption = oldOption | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOption);
    return oldOption;
}

void WebServer::addConnectionFd(int epollFd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    event.events |= EPOLLET;
    // 注册EPOLLONESHOT事件，保证一个socketFd同时只会有一个事件发生
    event.events |= EPOLLONESHOT;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event);
    setNonblocking(fd);
}

void WebServer::dispatchTask(int sockFd, TaskType taskType)
{
    if (taskType == WebServer::ReadTask)
    {
        (m_pConnections + sockFd)->init(sockFd, m_epollFd, HTTPConnection::Read);
    }
    else if (taskType == WebServer::WriteTask)
    {
        (m_pConnections + sockFd)->init(sockFd, m_epollFd, HTTPConnection::Write);
    }
    else
    {
        // error
    }
    m_theadPool.append(m_pConnections + sockFd);
}

// void WebServer::doWithRead(int sockFd)
// {
//     char buf[BUFFER_SIZE];
//     // ET模式
//     if (m_epollTriggerMode == true)
//     {
//         (m_pConnections + sockFd)->init(sockFd, m_epollFd, HTTPConnection::READ);
//         m_theadPool.append(m_pConnections + sockFd);
//     }
//     else // LT模式
//     {
//         memset(buf, '\0', BUFFER_SIZE);
//         int ret = recv(sockFd, buf, BUFFER_SIZE - 1, 0);
//         if (ret <= 0)
//         {
//             close(sockFd);
//             return ;
//         }
//         std::cout << "doWithRead(): 读取" << ret << "字节的内容。" << std::endl;
//     }
// }

// void WebServer::doWithWrite(int sockFd)
// {
//     (m_pConnections + sockFd)->init(sockFd, m_epollFd, HTTPConnection::READ);
//     m_theadPool.append(m_pConnections + sockFd);
// }

WebServer::WebServer(int port) : 
    m_listenFd(-1),
    m_port(port), 
    m_epollFd(-1),
    m_epollTriggerMode(true),
    m_theadPool()
{
    m_pConnections = new HTTPConnection[MAX_FD];
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
    m_epollFd = epoll_create(5); // 参数给内核一个提示，告诉事件表需要多大，现在已经被弃用，但是不要传0
    if (m_epollFd == -1)
    {
        std::cerr << "epoll_create(): 执行失败。" << std::endl;
        return ;
    }
    // m_listenFd注册epoll事件
    epoll_event event;
    event.data.fd = m_listenFd;
    event.events = EPOLLIN;
    event.events |= EPOLLET;
    epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_listenFd, &event);
    setNonblocking(m_listenFd);
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
            std::cerr << "errno: " << errno << std::endl;
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
                addConnectionFd(m_epollFd, connFd);
            }
            else if (events[i].events & EPOLLIN)
            {
                // doWithRead(sockFd);
                dispatchTask(sockFd, WebServer::ReadTask);
            }
            else if (events[i].events & EPOLLOUT)
            {
                // doWithWrite(sockFd);
                dispatchTask(sockFd, WebServer::WriteTask);
            }
            else
            {
                // do nothing
            }
        }
    }
}
