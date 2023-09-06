#ifndef _HTTPCONNECTION_H_
#define _HTTPCONNECTION_H_

class HttpConnection
{
public:
    HttpConnection() = default;
    ~HttpConnection() = default;
    void process();
    void setSocket(int socketFd)
    {
        m_socketFd = socketFd;
    }
private:
    int m_socketFd;
};
#endif