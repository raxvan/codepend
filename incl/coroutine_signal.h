
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

	public:
		bool wait(); // returns true if thread waited

	public:
		void add_to_arrivals();
		void arrive_and_continue();

	protected:
		std::atomic<int> m_counter { 1 };
		bool			 m_waiting = false;

		std::mutex				m_mutex;
		std::condition_variable m_wait;
	};
}