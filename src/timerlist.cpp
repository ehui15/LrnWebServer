#include "timerlist.h"

#include <iostream>

#include "httpconnection.h"

TimerList::~TimerList()
{
    Timer *pTimer = m_pHead;
    while (pTimer)
    {
        Timer *pNext = pTimer->m_pNext;
        delete pTimer;
        pTimer = pNext;
    }
}

void TimerList::addTimer(Timer *pTimer)
{
    if (!pTimer)
    {
        return ;
    }
    if (!m_pHead)
    {
        m_pHead = m_pTail = pTimer;
        std::cout << "addTimer: " << pTimer->m_expireTime << std::endl;
        return ;
    }
    if (pTimer->m_expireTime < m_pHead->m_expireTime)
    {
        pTimer->m_pNext = m_pHead;
        m_pHead->m_pPrev = pTimer;
        pTimer->m_pPrev = nullptr;
        m_pHead = pTimer;
        return ;
    }
    else
    {
        addTimer(pTimer, m_pHead);
    }
    std::cout << "addTimer: " << pTimer->m_expireTime << std::endl;
}

void TimerList::adjustTimer(Timer *pTimer, int delayTime)
{
    if (!pTimer)
    {
        return ;
    }
    pTimer->m_expireTime = time(NULL) + delayTime;
    std::cout << "adjustTimer: " << pTimer->m_expireTime << std::endl;
    if (pTimer->m_pNext == nullptr || pTimer->m_pNext->m_expireTime > pTimer->m_expireTime)
    {
        return ;
    }
    if (pTimer == m_pHead)
    {
        m_pHead = pTimer->m_pNext;
        pTimer->m_pPrev = nullptr;
        addTimer(pTimer, m_pHead);
    }
    else
    {
        pTimer->m_pPrev->m_pNext = pTimer->m_pNext;
        pTimer->m_pNext->m_pPrev = pTimer->m_pPrev;
        addTimer(pTimer, pTimer->m_pNext);
    }
}

void TimerList::tick()
{
    if (!m_pHead)
    {
        return ;
    }
    time_t currentTime = time(NULL);
    std::cout << "current time: " << currentTime << std::endl;
    Timer *p = m_pHead;
    while (p)
    {
        if (p->m_expireTime > currentTime)
            break;
        std::cout << "delete timer: " << p->m_expireTime << std::endl;
        p->m_pConnection->closeSocket();
        std::cout << "closeSocket: " << p->m_expireTime << std::endl;
        m_pHead = p->m_pNext;
        if (m_pHead)
            m_pHead->m_pPrev = nullptr;
        delete p;
        p = m_pHead;
    }
}

void TimerList::addTimer(Timer *pTimer, Timer *pHead)
{
    Timer *p = pHead;
    while (p != m_pTail && p->m_expireTime < pTimer->m_expireTime) p++;
    if (p == m_pTail)
    {
        pTimer->m_pPrev = m_pTail;
        pTimer->m_pNext = nullptr;
        m_pTail->m_pNext = pTimer;
        m_pTail = pTimer;
    }
    else
    {
        pTimer->m_pPrev = p->m_pPrev;
        pTimer->m_pNext = p;
        p->m_pPrev = pTimer;
    }
}
