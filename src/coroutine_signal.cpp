
#include <coroutine_signal.h>

namespace cdp
{
	cosignal::~cosignal()
	{
		std::unique_lock<std::mutex> _(m_mutex);
		CDP_ASSERT(m_arrived.load() == m_expected && m_waiting == false);
	}

	cosignal::cosignal(const uint32_t expected)
	{
		reset(expected);
	}

	void cosignal::reset(const uint32_t expected)
	{
		std::unique_lock<std::mutex> _(m_mutex);
		CDP_ASSERT(m_arrived.load() == m_expected);

		m_arrived.store(0);
		m_expected = int(expected);
	}

	bool cosignal::wait()
	{
		std::unique_lock<std::mutex> lk(m_mutex);
		int							 arrived = m_arrived.load();
		CDP_ASSERT(m_expected != 0 && arrived <= m_expected);
		if (arrived != m_expected)
		{
			m_waiting = true;
			m_wait.wait(lk);
			m_waiting = false;
			return true;
		}
		return false;
	}

	void cosignal::arrive_and_continue()
	{
		int r = ++m_arrived;
		CDP_ASSERT(r <= m_expected);
		if (r == m_expected)
		{
			std::unique_lock<std::mutex> _(m_mutex);
			if (m_waiting)
				m_wait.notify_one();
		}
	}
}