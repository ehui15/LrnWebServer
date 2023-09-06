#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <queue>
#include <pthread.h>
#include <cstdio>
#include <exception>
#include <mutex>

#include "sem.h"

template<typename T>
class ThreadPool
{
public:
    // 我电脑CPU是i5-12500H，有12个核心
    ThreadPool(int threadNumber = 12, int maxRequests = 10000);
    ~ThreadPool();
    bool append(T *pTask);
private:
    static void* work(void *arg);
    void run();

    // int m_threadNumber;
    int m_maxRequests;
    std::queue<T *> m_requestQueue;
    pthread_t *m_threadIds;
    std::mutex m_mutex;
    Sem m_sem;
    bool m_threadStop;
};

template <typename T>
ThreadPool<T>::ThreadPool(int threadNumber, int maxRequests) : 
    // m_threadNumber(threadNumber),
    m_maxRequests(maxRequests),
    m_threadIds(nullptr),
    m_threadStop(false)
{
    if ((threadNumber <= 0) || (maxRequests <= 0))
    {
        throw std::exception();
    }
    m_threadIds = new pthread_t[threadNumber];
    if (!m_threadIds)
    {
        throw std::exception();
    }
    for (int i = 0; i < threadNumber; i++)
    {
        // work()必须为静态函数
        if (pthread_create(&m_threadIds[i], NULL, work, this) != 0)
        {
            delete []m_threadIds;
            throw std::exception();
        }
        // 设置为脱离线程
        if (pthread_detach(m_threadIds[i]))
        {
            delete []m_threadIds;
            throw std::exception();
        }
    }

}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
    delete []m_threadIds;
}

template <typename T>
bool ThreadPool<T>::append(T *pTask)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_requestQueue.size() > m_maxRequests)
        {
            return false;
        }
        m_requestQueue.push(pTask);
    }
    m_sem.post();
    return true;
}

template <typename T>
void *ThreadPool<T>::work(void *arg)
{
    ThreadPool *pThreadPool = (ThreadPool *)arg;
    // work()实例无关访问不到实例相关的成员，所以需要定义run()来实现线程的功能
    pThreadPool->run();
    // 返回值可以使用pthread_join()获取到
    return nullptr;
}

template <typename T>
void ThreadPool<T>::run()
{
    while (!m_threadStop)
    {
        m_sem.wait();
        T *pTask = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_requestQueue.empty())
            {
                continue;
            }
            pTask = m_requestQueue.front();
            m_requestQueue.pop();
        }
        if (!pTask)
            continue;
        pTask->process();
    }
}
#endif