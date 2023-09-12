#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#include "threadpool.h"

class HTTPConnection;

class WebServer
{
private:
    // 任务类型
    enum TaskType
    {
        ReadTask = 0,
        WriteTask
    };
    int setNonblocking(int fd);
    void addConnectionFd(int epollFd, int fd);
    // void doWithRead(int sockFd);
    // void doWithWrite(int sockFd);
    void dispatchTask(int sockFd, TaskType taskType);
    int m_listenFd;
    int m_port;
    int m_epollFd;
    int m_epollTriggerMode; // true代表ET，false代表LT
    ThreadPool<HTTPConnection> m_theadPool;
    HTTPConnection *m_pConnections;
public:
    WebServer(int port);
    ~WebServer();
    void init();
    void eventLoop();
};

#endif