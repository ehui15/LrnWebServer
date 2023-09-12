#ifndef _HTTPCONNECTION_H_
#define _HTTPCONNECTION_H_

#include <linux/limits.h>

const int READ_BUFFER_SIZE = 2048;
const int WRITE_BUFFER_SIZE = 2048;

class HTTPConnection
{
public:
    // 表示HTTP连接工作状态
    enum WorkState
    {
        Read = 0,   // 读数据
        Write       // 写数据
    };
    // 表示解析请求的结果
    enum RequestResult
    {
        NoRequest = 0,  // 请求不完整
        GetRequest, // 获取了一个完整的请求
        BadRequest  // 请求格式错误
    };

    // 表示状态机解析HTTP请求报文的状态
    enum ParseState
    {
        RequestLine = 0,   // 解析请求行
        HeaderLine         // 解析头部行
    };

    HTTPConnection();
    ~HTTPConnection();
    void processRead();
    RequestResult readAndParse();
    void init(int socketFd, int epollFd, WorkState workState)
    {
        m_socketFd = socketFd;
        m_epollFd = epollFd;
        m_workState = workState;
    }
    void resetEpollEvent(int e);
    bool parseLine(char *pBuffer, int &parseIndex);
    RequestResult parseRequestLine(char *pLineBuffer);
    RequestResult parseHeaderLine(char *pLineBuffer);
    void prepareWrite();
    void closeSocket();
    void processWrite();
    WorkState m_workState;
    void clear();
private:
    int m_socketFd;
    int m_epollFd;
    char m_readBuffer[READ_BUFFER_SIZE];
    int m_readBufferSize;
    char m_resourcePath[PATH_MAX];
    char m_writeBuffer[WRITE_BUFFER_SIZE];
    int m_writeBufferSize;
};
#endif