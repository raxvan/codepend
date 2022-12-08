
#include <coroutine_signal.h>

namespace cdp
{
	cosignal::cosignal()
	{
	}

	cosignal::~cosignal()
	{
	}

	void cosignal::wait()
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		int							 i = --m_counter;
		CDP_ASSERT(i >= 0);
		if (i != 0)
		{
			m_waiting = true;
			m_wait.wait(lk);
			m_waiting = false;
		}

		m_counter++;
	}

	bool cosignal::active()
	{
		auto i = m_counter.load();
		CDP_ASSERT(i >= 0);
		return i > 1;
	}

	void cosignal::acquire()
	{
		int r = m_counter++;
		CDP_ASSERT(r > 0);
	}

	void cosignal::release_and_continue()
	{
		int r = --m_counter;
		CDP_ASSERT(r >= 0);

		if (r == 0)
		{
			std::unique_lock<std::mutex> _(m_mutex);
			if (m_counter == 0 && m_waiting)
				m_wait.notify_one();
		}
	}
}
