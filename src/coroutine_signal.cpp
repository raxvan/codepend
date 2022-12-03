
#include <coroutine_signal.h>

namespace cdp
{
	cosignal::cosignal()
	{
	}

	cosignal::~cosignal()
	{
	}

	void cosignal::reset()
	{
		std::unique_lock<std::mutex> _(m_mutex);
		CDP_ASSERT(m_counter == 0 && m_waiting == false);
		m_counter.store(1);
	}

	bool cosignal::wait()
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		int i = --m_counter;
		CDP_ASSERT(i >= 0);
		if (i != 0)
		{
			m_waiting = true;
			m_wait.wait(lk);
			m_waiting = false;
			return true;
		}

		return false;
	}

	void cosignal::mark()
	{
		m_counter++;
	}

	void cosignal::arrive_and_continue()
	{
		int r = --m_counter;
		CDP_ASSERT(r >= 0);
		if (r == 0)
		{
			std::unique_lock<std::mutex> _(m_mutex);
			CDP_ASSERT(m_counter == 0);
			if (m_waiting)
				m_wait.notify_one();
		}
	}
}