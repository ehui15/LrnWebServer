#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#include "threadpool.h"
#include "timerlist.h"

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
    void addFd(int epollFd, int fd, bool oneShot);
    void dispatchTask(int sockFd, TaskType taskType);

    static void alarmHandler(int signal);
    static int m_pipeFd[2];

    int m_listenFd;
    int m_port;
    int m_epollFd;
    int m_epollTriggerMode; // true代表ET，false代表LT
    ThreadPool<HTTPConnection> m_theadPool;
    HTTPConnection *m_pConnections;
    TimerList m_timerList;
public:
    WebServer(int port);
    ~WebServer();
    void init();
    void eventLoop();
};

#endif