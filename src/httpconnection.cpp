#include "httpconnection.h"

#include <memory.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <iostream>
#include <fstream>

const int LINE_BUFFER_SIZE = 1024;

HTTPConnection::HTTPConnection()
{
}

HTTPConnection::~HTTPConnection()
{
}

void HTTPConnection::processRead()
{
    RequestResult requestResult = readAndParse();
    if (requestResult == GetRequest)
    {
        prepareWrite();
        // 注册EPOLLOUT事件
        resetEpollEvent(EPOLLOUT);
    }
    else
    {
        closeSocket();
    }
}

HTTPConnection::RequestResult HTTPConnection::readAndParse()
{
    // ET模式下可读事件只会触发一次，所以需要循环读，把缓存中的数据全部读出
    while (1)
    {
        int readBytes = recv(m_socketFd, m_readBuffer + m_readBufferSize, READ_BUFFER_SIZE - m_readBufferSize, 0);
        if (readBytes < 0)
        {
            // 表示数据已经全部读完
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                std::cout << "doWithRead(): 读完。" << std::endl;
                break;
            }
        }
        else if (readBytes == 0)
        {
            std::cerr << "doWithRead(): 远程客户已经关闭连接。" << std::endl;
            closeSocket();
        }
        else
        {
            m_readBufferSize += readBytes;
            std::cout << "doWithRead(): 读取" << readBytes << "字节的内容。" << std::endl;
        }
    }
    // 解析
    char lineBuffer[LINE_BUFFER_SIZE];
    int parseIndex = 0;
    ParseState state = RequestLine;
    while (parseLine(lineBuffer, parseIndex))
    {
        switch (state)
        {
            case RequestLine:
            {
                RequestResult res = parseRequestLine(lineBuffer);
                if (res == BadRequest)
                {
                    return BadRequest;
                }
                else
                {
                    state = HeaderLine;
                }
                break;
            }
            case HeaderLine:
            {
                RequestResult res = parseHeaderLine(lineBuffer);
                if (res == GetRequest)
                    return GetRequest;
                // todo
                break;
            }
        }
    }
    return NoRequest;
}

void HTTPConnection::resetEpollEvent(int e)
{
    epoll_event event;
    event.data.fd = m_socketFd;
    event.events = e | EPOLLET | EPOLLONESHOT;
    epoll_ctl(m_epollFd, EPOLL_CTL_MOD, m_socketFd, &event);
}

bool HTTPConnection::parseLine(char *pBuffer, int &parseIndex)
{
    if (parseIndex >= m_readBufferSize)
        return false;
    int bufferIndex = 0;
    while (parseIndex <= m_readBufferSize && m_readBuffer[parseIndex] != '\r')
    {
        pBuffer[bufferIndex++] = m_readBuffer[parseIndex++];
    }
    if (m_readBuffer[parseIndex + 1] != '\n')
        return false;
    // 读取\r\n
    pBuffer[bufferIndex++] = m_readBuffer[parseIndex++];
    pBuffer[bufferIndex++] = m_readBuffer[parseIndex++];
    pBuffer[bufferIndex] = '\0';
    return true;
}

HTTPConnection::RequestResult HTTPConnection::parseRequestLine(char *pLineBuffer)
{
    // 检查格式
    char *pSpace = strpbrk(pLineBuffer, " ");
    if (!pSpace)
        return BadRequest;
    // 检测GET关键字
    *pSpace++ = '\0';
    if (strcasecmp(pLineBuffer, "GET") != 0)
    {
        return BadRequest;
    }
    // 获取URL
    char *pUrl = pSpace;
    pSpace = strpbrk(pUrl, " ");
    if (!pSpace)
        return BadRequest;
    *pSpace = '\0';
    // URL以http开头
    char *pBegin = nullptr;
    if (*pUrl == 'h')
    {
        // 找到第三个'/'，例：http://xxx/xxxxxxx
        pBegin = pUrl;
        int num = 0;
        while (1)
        {
            if (*pBegin == '/')
            {
                num += 1;
                if (num == 3)
                    break;
            }
            pBegin += 1;
        }
    }
    else
    {
        pBegin = pUrl;
    }
    strcpy(m_resourcePath, pBegin);
    return NoRequest;
}

HTTPConnection::RequestResult HTTPConnection::parseHeaderLine(char *pLineBuffer)
{
    if (strcmp(pLineBuffer, "\r\n") == 0)
        return GetRequest;
    else
        return NoRequest;
}

void HTTPConnection::prepareWrite()
{
    char *pWrite = m_writeBuffer;
    // 构造响应行
    int writeBytes = sprintf(pWrite, "HTTP/1.1 200 OK\r\n");
    pWrite += writeBytes;
    m_writeBufferSize += writeBytes;
    // 构造响应头
    if (strcasecmp(m_resourcePath, "/") == 0)
    {
        strcat(m_resourcePath, "index.html");
    }
    char filePath[PATH_MAX] = "../resource";
    strcat(filePath, m_resourcePath);
    std::ifstream fileStream(filePath, std::ios::binary);
    if (!fileStream.is_open())
    {
        std::cerr << "HTTPConnection::processWrite(): 无法打开文件。" << std::endl;
        fileStream.close();
        return ;
    }
    fileStream.seekg(0, std::ios::end);
    std::streampos fileSize = fileStream.tellg();
    if (fileSize == -1)
    {
        std::cerr << "HTTPConnection::processWrite(): 无法获取文件大小。" << std::endl;
        return ;
    }
    writeBytes = sprintf(pWrite, "Content-Length: %d\r\n", (int)fileSize);
    pWrite += writeBytes;
    m_writeBufferSize += writeBytes;
    writeBytes = sprintf(pWrite, "Content-Type: %s\r\n", "text/html");
    pWrite += writeBytes;
    m_writeBufferSize += writeBytes;
    writeBytes = sprintf(pWrite, "Connection: %s\r\n", "close");
    pWrite += writeBytes;
    m_writeBufferSize += writeBytes;
    writeBytes = sprintf(pWrite, "\r\n");
    pWrite += writeBytes;
    m_writeBufferSize += writeBytes;
    // 添加内容
    fileStream.seekg(0, std::ios::beg);
    fileStream.read(pWrite, fileSize);
    fileStream.close();
    m_writeBufferSize += fileSize;
}

void HTTPConnection::closeSocket()
{
    epoll_ctl(m_epollFd, EPOLL_CTL_DEL, m_socketFd, 0);
    close(m_socketFd);
    clear();
}

void HTTPConnection::processWrite()
{
    int writeBytes = 0;

    while (writeBytes != m_writeBufferSize)
    {
        int bytes = send(m_socketFd, m_writeBuffer + writeBytes, m_writeBufferSize - writeBytes, 0);
        if (bytes < 0)
        {
            std::cerr << "processWrite(): send()出错。" << std::endl;
            return ;
        }
        writeBytes += bytes;
    }
    resetEpollEvent(EPOLLIN);
    std::cout << "processWrite(): 完成。" << std::endl;
}

void HTTPConnection::clear()
{
    m_readBufferSize = 0;
    m_writeBufferSize = 0;
}
