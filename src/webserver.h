#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#include "threadpool.h"

class HttpConnection;

class WebServer
{
private:
    int setNonblocking(int fd);
    void addFd(int epollFd, int fd, bool enableET);
    void doWithRead(int sockFd);

    int m_listenFd;
    int m_port;
    int m_epollFd;
    int m_epollTriggerMode; // true代表ET，false代表LT
    ThreadPool<HttpConnection> m_theadPool;
    HttpConnection *m_pConnections;
public:
    WebServer(int port);
    ~WebServer();
    void init();
    void eventLoop();
};

#endif