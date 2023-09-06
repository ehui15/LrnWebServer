#include <semaphore.h>
#include <exception>

class Sem
{
public:
    Sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    ~Sem()
    {
        sem_destroy(&m_sem);
    }
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }
private:
    sem_t m_sem;
};