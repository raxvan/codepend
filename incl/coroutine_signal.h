
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
		void reset();

		bool wait(); // returns true if thread waited

	protected:
		friend struct coroutine;

		void mark();
		void arrive_and_continue();

	protected:
		std::atomic<int> m_counter { 1 };
		bool			 m_waiting = false;

		std::mutex				m_mutex;
		std::condition_variable m_wait;
	};
}