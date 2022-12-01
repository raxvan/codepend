
#pragma once

#include "codepend_config.h"

namespace cdp
{

	struct cosignal
	{
	public:
		~cosignal();

	public:
		cosignal() = default;
		cosignal(const uint32_t expected);
		void reset(const uint32_t expected);

		bool wait(); // returns true if thread waited

	protected:
		friend struct coroutine;

		void arrive_and_continue();

	protected:
		std::atomic<int> m_arrived { 0 };
		int				 m_expected { 0 };
		bool			 m_waiting = false;

		std::mutex				m_mutex;
		std::condition_variable m_wait;
	};
}