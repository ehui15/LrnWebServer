#include "httpconnection.h"

#include <memory.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <iostream>
#include <fstream>

#include "timerlist.h"

const int LINE_BUFFER_SIZE = 1024;

HTTPConnection::HTTPConnection()
{
}

HTTPConnection::~HTTPConnection()
{
}

void HTTPConnection::processRead()
{
    readRequest();
    RequestResult requestResult = parseRequest();
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

void HTTPConnection::readRequest()
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
                std::cout << "readRequest(): 读完。" << std::endl;
                break;
            }
        }
        else if (readBytes == 0)
        {
            std::cerr << "readRequest(): 远程客户已经关闭连接。" << std::endl;
            closeSocket();
        }
        else
        {
            m_readBufferSize += readBytes;
            std::cout << "readRequest(): 读取" << readBytes << "字节的内容。" << std::endl;
        }
    }
}

HTTPConnection::RequestResult HTTPConnection::parseRequest()
{
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
    std::cout << "URL: " << m_resourcePath << std::endl;
    // 构造响应头
    if (strcasecmp(m_resourcePath, "/") == 0)
    {
        strcat(m_resourcePath, "index.html");
    }
    // 添加文件后缀
    std::string url(m_resourcePath);
    if (url.find('.') == std::string::npos)
    {
        strcat(m_resourcePath, ".html");
    }
    // 判断文件是否存在
    char filePath[PATH_MAX] = "../resource";
    strcat(filePath, m_resourcePath);
    strcpy(m_resourcePath, filePath);
    std::ifstream fileStream(m_resourcePath);
    if (fileStream.good())
    {
        // 构造响应头
        addResponseLine("HTTP/1.1 200 OK");
        // 获取文件类型
        std::string contentType;
        std::string filePath(m_resourcePath);
        size_t lastDotPos = filePath.find_last_of('.');
        if (lastDotPos != std::string::npos)
        {
            contentType = filePath.substr(lastDotPos + 1);
        }
        if (contentType == "ico")
        {
            contentType = std::string("text/x-icon");
        }
        else if (contentType == "html")
        {
            contentType = std::string("text/html; charset=utf-8");
        }
        else if (contentType == ".jpeg" || contentType == "jpg")
        {
            contentType = std::string("image/jpeg");
        }
        else
        {
            contentType = std::string("text/plain; charset=utf-8");
        }
        addResponseHead("Content-Type", contentType);
        // 获取文件大小
        fileStream.seekg(0, std::ios::end);
        std::streampos fileSize = fileStream.tellg();
        if (fileSize == -1)
        {
            std::cerr << "prepareWrite(): 无法获取文件大小。" << std::endl;
            return ;
        }
        addResponseHead("Content-Length", std::to_string(fileSize));
        addResponseLine("");
    }
    else
    {
        addResponseLine("HTTP/1.1 404 Not Found");
        addResponseHead("Content-type", "text/html; charset=utf-8");
        addResponseHead("Content-Length", "663");
        addResponseLine("");
        strcpy(m_resourcePath, "../resource/404.html");
    }
    fileStream.close();
}

void HTTPConnection::closeSocket()
{
    epoll_ctl(m_epollFd, EPOLL_CTL_DEL, m_socketFd, 0);
    close(m_socketFd);
    m_pTimer = nullptr;
    clearBuffer();
}

void HTTPConnection::processWrite()
{
    // 发送报文头
    int writeBytes = 0;
    while (writeBytes != m_writeBufferSize)
    {
        int bytes = send(m_socketFd, m_writeBuffer + writeBytes, m_writeBufferSize - writeBytes, 0);
        std::cout << "send(): " << bytes << " 字节报文头数据已发送。" << std::endl;
        if (bytes < 0)
        {
            std::cerr << "processWrite(): send()出错。" << std::endl;
            break;
        }
        writeBytes += bytes;
    }
    // 发送报文内容
    std::ifstream file(m_resourcePath, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "processWrite(): 无法打开文件。" << std::endl;
        resetEpollEvent(EPOLLIN);
        return ;
    }
    char buffer[WRITE_BUFFER_SIZE];
    while (!file.eof())
    {
        file.read(buffer, sizeof(buffer));
        int bytesRead = file.gcount();
        int bytes = send(m_socketFd, buffer, bytesRead, 0);
        if (bytes < 0)
        {
            std::cerr << "processWrite(): send()出错。" << std::endl;
            break;
        }
        std::cout << "send(): " << bytes << " 字节报文内容已发送。" << std::endl;
    }
    file.close();

    resetEpollEvent(EPOLLIN);
    std::cout << "processWrite(): 完成。" << std::endl;
    clearBuffer();
}

void HTTPConnection::clearBuffer()
{
    m_readBufferSize = 0;
    m_writeBufferSize = 0;
    m_resourcePath[0] = '\0';
}

void HTTPConnection::addResponseHead(const std::string &lineHead, const std::string &lineContent)
{
    int bytes = 0;
    if ((bytes = sprintf(m_writeBuffer + m_writeBufferSize, (lineHead + ": %s\r\n").c_str(), lineContent.c_str())) < 0)
    {
        std::cerr << "addResponseHead(): sprintf()出错。" << std::endl;
        return ;
    }
    m_writeBufferSize += bytes;
}

void HTTPConnection::addResponseLine(const std::string &line)
{
    int bytes = 0;
    if ((bytes = sprintf(m_writeBuffer + m_writeBufferSize, "%s\r\n", line.c_str())) < 0)
    {
        std::cerr << "addResponseLine(): sprintf()出错。" << std::endl;
        return ;
    }
    m_writeBufferSize += bytes;
}
