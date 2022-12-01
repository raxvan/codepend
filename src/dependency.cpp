
#include <coroutine_pipe.h>

namespace cdp
{
	void dependency::resolve(const uint32_t payload)
	{
		CDP_ASSERT(m_owner != nullptr);
		coroutine co;
		{
			std::lock_guard<threading::spin_lock> _(*this);
			CDP_ASSERT(_isresolved_locked() == false && payload != std::numeric_limits<uint32_t>::max());
			m_payload = payload;
			co.attach(waiting_list);
		}
		while(co.handle)
		{
			CDP_ASSERT(co.handle.promise().waiting_for == this);
			co.handle.promise().waiting_for = nullptr;
			auto next = coroutine(std::move(co.handle.promise().next));
			m_owner->push_back(std::move(co));
			co = std::move(next);
		}
	}

	void 	 dependency::resolve_in_frame(const uint32_t payload)
	{
		CDP_ASSERT(m_owner != nullptr);
		coroutine co;
		{
			std::lock_guard<threading::spin_lock> _(*this);
			CDP_ASSERT(_isresolved_locked() == false && payload != std::numeric_limits<uint32_t>::max());
			m_payload = payload;
			co.attach(waiting_list);
		}
		while(co.handle)
		{
			CDP_ASSERT(co.handle.promise().waiting_for == this);
			co.handle.promise().waiting_for = nullptr;
			auto next = coroutine(std::move(co.handle.promise().next));
			m_owner->execute_frame(std::move(co));
			co = std::move(next);
		}
	}

	dependency::~dependency()
	{
		if(waiting_list)
		{
			coroutine co(waiting_list);
			do
			{
				CDP_ASSERT(co.handle.promise().waiting_for == this);
				co.handle.promise().waiting_for = nullptr;
				auto next = coroutine(std::move(co.handle.promise().next));
				m_owner->push_back(std::move(co));
				co = std::move(next);
			}
			while(co.handle);
		}

	}

	void dependency::set(coroutine_pipe& p)
	{
		CDP_ASSERT(m_owner == nullptr);
		m_owner = &p;
	}
	void dependency::set(coroutine_pipe* p)
	{
		CDP_ASSERT(m_owner == nullptr);
		m_owner = p;
	}

	bool     dependency::resolved()
	{
		std::lock_guard<threading::spin_lock> _(*this);
		return _isresolved_locked();
	}

	coroutine_pipe& dependency::pipe()
	{
		CDP_ASSERT(m_owner != nullptr);
		return *m_owner;
	}

	bool 	dependency::owned(const coroutine_pipe* p) const
	{
		return m_owner == p;
	}

	/*uint32_t dependency::_resolve(const uint32_t payload)
	{
		std::lock_guard<threading::spin_lock> _(*this);
		return _resolve_locked(payload);
	}

	void dependency::_resolve_locked(const uint32_t payload)
	{
		
		//dependency must not be solved from another place

		m_payload = payload;

		//detach tasks to be processed from the call place
		auto r = m_first_task;
		m_first_task = std::numeric_limits<uint32_t>::max();
		return r;
	}*/

	bool 	 dependency::_isresolved_locked() const
	{
		return m_payload != std::numeric_limits<uint32_t>::max();
	}

	
}
