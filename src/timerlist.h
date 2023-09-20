#ifndef _TIMELIST_H_
#define _TIMELIST_H_

#include <ctime>

class HTTPConnection;

class Timer
{
public:
    Timer(int delayTime) : m_pConnection(nullptr), m_pPrev(nullptr), m_pNext(nullptr)
    {
        m_expireTime = time(NULL) + delayTime;
    }
    ~Timer() = default;
    Timer *m_pPrev;
    Timer *m_pNext;
    HTTPConnection *m_pConnection;    // HTTP连接指针
    time_t m_expireTime;              // 定时器到期的绝对时刻
};

class TimerList
{
public:
    TimerList() : m_pHead(nullptr), m_pTail(nullptr) {}
    ~TimerList();
    void addTimer(Timer *pTimer);
    void adjustTimer(Timer *pTimer, int delayTime);
    void tick();
private:
    void addTimer(Timer *pTimer, Timer *pHead);
    Timer *m_pHead;
    Timer *m_pTail;
};
#endif