#include "httpconnection.h"

#include <memory.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <iostream>

const int BUFFER_SIZE = 1024;

void HttpConnection::process()
{
    char buf[BUFFER_SIZE];
    // ET模式下可读事件只会触发一次，所以需要循环读，把缓存中的数据全部读出
    while (1)
    {
        memset(buf, '\0', BUFFER_SIZE);
        int ret = recv(m_socketFd, buf, BUFFER_SIZE - 1, 0);
        if (ret < 0)
        {
            // 表示数据已经全部读完
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                std::cout << "doWithRead(): 读完。" << std::endl;
                break;
            }
            close(m_socketFd);
        }
        else if (ret == 0)
        {
            close(m_socketFd);
        }
        else
        {
            std::cout << "doWithRead(): 读取" << ret << "字节的内容。" << std::endl;
        }
    }
}
