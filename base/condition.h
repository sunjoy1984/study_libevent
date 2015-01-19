#ifndef _COMM_CONDITION_H
#define _COMM_CONDITION_H

#include <pthread.h>
#include <sys/time.h>
#include <cassert>
#include "base/mutex.h"
using namespace std;

namespace comm {
class Condition
{
public:
	explicit Condition(Mutex& mutex): m_mutex(mutex)
	{
		pthread_cond_init(&m_cond, NULL);
	}

	~Condition()
	{
		pthread_cond_destroy(&m_cond);
	}

	void Wait()
	{
		pthread_cond_wait(&m_cond, m_mutex.getMutex());
	}

	int TimedWait(long ms)
	{
		assert(ms >= 0);
		struct timespec tsp;
		MakeTimeout(&tsp, ms);
		return pthread_cond_timedwait(&m_cond, m_mutex.getMutex(), &tsp);
	}

	void Notify()
	{
		pthread_cond_signal(&m_cond);
	}

	void NotifyAll()
	{
		pthread_cond_broadcast(&m_cond);
	}
private:
	void MakeTimeout(struct timespec* pTsp, long ms)
	{
		struct timeval now;
		gettimeofday(&now, 0);
		pTsp->tv_sec = now.tv_sec;
		pTsp->tv_nsec = now.tv_usec*1000;

		pTsp->tv_sec += ms/1000;
		pTsp->tv_nsec += (ms%1000)*1000000;

	}
private:
	Mutex& m_mutex;
	pthread_cond_t m_cond;

};

class WaitEvent
{
public:
    WaitEvent() : _signaled(false)
    {
        pthread_mutex_init(&_lock, NULL);
        pthread_cond_init(&_cond, NULL);
    }
    ~WaitEvent()
    {
        pthread_mutex_destroy(&_lock);
        pthread_cond_destroy(&_cond);
    }

    void Wait()
    {
        pthread_mutex_lock(&_lock);
        while (!_signaled)
        {
            pthread_cond_wait(&_cond, &_lock);
        }
        _signaled = false;
        pthread_mutex_unlock(&_lock);
    }

    void Signal()
    {
        pthread_mutex_lock(&_lock);
        _signaled = true;
        pthread_cond_signal(&_cond);
        pthread_mutex_unlock(&_lock);
    }

private:
    pthread_mutex_t _lock;
    pthread_cond_t _cond;
    bool _signaled;
};


}

#endif
