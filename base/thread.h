#ifndef _COMM_THREAD_H
#define _COMM_THREAD_H

#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <iostream>


namespace comm {
using namespace std;

typedef void* (*THREAD_RUTINE)(void*);

class Thread
{
	public:
		Thread():  m_bStarted(false), m_threadID(0)
		{
		}

		~Thread()
		{
		}

		void Start(THREAD_RUTINE routine, void* pParams)
		{
			assert(!m_bStarted);
			m_bStarted = true;
			if(0 != pthread_create(&m_threadID, NULL, routine, pParams))
			{
				cerr<< "create thread failure"<<endl;
				abort();
			}
		}

		int Join()
		{
			assert(m_bStarted);
			return pthread_join(m_threadID, NULL);
		}

		int Detach()
		{
			assert(m_bStarted);
			return pthread_detach(m_threadID);
		}
	private:
		bool m_bStarted;
		pthread_t m_threadID;
};

}

#endif
