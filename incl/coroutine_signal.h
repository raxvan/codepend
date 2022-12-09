
#pragma once

#include "codepend_config.h"

namespace cdp
{

	struct cosignal
	{
	public:
		~cosignal();

	public:
		cosignal();

	public:
		void wait(); // for release

	public:
		bool active();//true if number of acquires != 0

	public:
		void acquire();
		void release_and_continue();

	protected:
		std::atomic<int> m_counter { 1 };
		bool			 m_waiting = false;

		std::mutex				m_mutex;
		std::condition_variable m_wait;
	};


	struct signal_lock
	{
	public:
		inline signal_lock(cosignal& s)
			:signal(&s)
		{
			s.acquire();
		}
		inline signal_lock(cosignal* s)
			:signal(s)
		{
			if(s != nullptr)
				s->acquire();
		}
		inline ~signal_lock()
		{
			if(signal != nullptr)
				signal->release_and_continue();
		}
	protected:
		cosignal* signal;
	};
}
