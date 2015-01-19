#ifndef _COMM_THREAD_POOL_H
#define _COMM_THREAD_POOL_H

#include <vector>
#include <queue>
#include <string.h>

#include "base/thread.h"
#include "base/mutex.h"
#include "base/condition.h"
#include "base/closure.h"

namespace comm {
using namespace std;

template <int Flag>
class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();

	void Run(int maxThread, int max_queue_size);
	void Submit(Closure<void>* job);
private:
	static void* RunInThread(void* p);
private:
	vector<Thread> m_threadPool;
	bool m_bRunning;
	std::deque<Closure<void>* > m_queue;
	Mutex m_mutex;
	Condition m_cond_consumer;
	Condition m_cond_producer;
	int m_max_queue_size;
};

typedef ThreadPool<1> MainThreadPool;

}

namespace comm{

template <int Flag>
ThreadPool<Flag>::ThreadPool():
	m_threadPool(0),
	m_bRunning(false),
	m_queue(0),
	m_cond_consumer(m_mutex),
	m_cond_producer(m_mutex),
	m_max_queue_size(0)
{
	m_bRunning = false;
}

template <int Flag>
ThreadPool<Flag>::~ThreadPool()
{
	m_bRunning = false;
	m_cond_consumer.NotifyAll();
	m_cond_producer.NotifyAll();
	for(int i=0; i<m_threadPool.size(); i++)
	{
		m_threadPool[i].Join();
	}
}

template <int Flag>
void ThreadPool<Flag>::Run(int maxThread, int max_queue_size)
{
	assert( maxThread>0);
	assert( !m_bRunning);
	m_bRunning = true;
	m_threadPool.reserve(maxThread);
	m_max_queue_size = max_queue_size;
	for(int i=0; i<maxThread; i++)
	{
		m_threadPool.push_back(Thread());
		m_threadPool[i].Start(RunInThread, this);
	}

}

template <int Flag>
void ThreadPool<Flag>::Submit(Closure<void>* job)
{
	assert(NULL != job);
	SafeMutex lock(m_mutex);
    while (m_queue.size() > m_max_queue_size && m_bRunning) {
        m_cond_producer.Wait();
    }
	m_queue.push_back(job);
	m_cond_consumer.Notify();
}

template <int Flag>
void* ThreadPool<Flag>:: RunInThread(void* p)
{

	assert(NULL != p);
	ThreadPool* pSelf = (ThreadPool*)p;
	while(pSelf->m_bRunning)
	{
		Closure<void> * job = NULL;

		{
			SafeMutex lock(pSelf->m_mutex);
			while(pSelf->m_queue.empty() && pSelf->m_bRunning)
			{
				pSelf->m_cond_consumer.Wait();
			}
			if(pSelf->m_bRunning)
			{
				job = pSelf->m_queue.front();
				pSelf->m_queue.pop_front();
				pSelf->m_cond_producer.Notify();
			}
		}

		if(job)
		{
			job->Run();
		}
	}

	return NULL;
}

}

#endif
