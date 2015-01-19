#ifndef _COMM_MUTEX_H
#define _COMM_MUTEX_H

#include <pthread.h>
#include <assert.h>

namespace comm{
class Mutex
{
	public:
		Mutex()
		{
			pthread_mutex_init(&m_mutex, NULL);
		}

		~Mutex()
		{
			pthread_mutex_destroy(&m_mutex);
		}

		void lock()
		{
			pthread_mutex_lock(&m_mutex);
		}

		void unlock()
		{
			pthread_mutex_unlock(&m_mutex);
		}

		pthread_mutex_t* getMutex()
		{
			return &m_mutex;
		}

	private:
		pthread_mutex_t m_mutex;
};


class SafeMutex
{
	public:
		explicit SafeMutex(Mutex& mutex): m_mutex(mutex)
	{
		m_mutex.lock();
	}

		~SafeMutex()
		{
			m_mutex.unlock();
		}

	private:
		Mutex& m_mutex;
};

}

#endif
