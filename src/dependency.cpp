
#include <coroutine_pipe.h>

namespace cdp
{
	void dependency::resolve(const uint32_t payload)
	{
		uint32_t tasks = _resolve(payload);
		CDP_ASSERT(m_owner != nullptr);
		m_owner->pipe_tasks_list(tasks, [o = m_owner](coroutine&& co){
			o->push_back(std::move(co));
		});
	}

	void 	 dependency::resolve_in_frame(const uint32_t payload)
	{
		uint32_t tasks = _resolve(payload);
		CDP_ASSERT(m_owner != nullptr);
		m_owner->pipe_tasks_list(tasks, [o = m_owner](coroutine&& co){
			o->execute_frame(std::move(co));
		});
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

	uint32_t dependency::_resolve(const uint32_t payload)
	{
		std::lock_guard<threading::spin_lock> _(*this);
		return _resolve_locked(payload);
	}

	uint32_t dependency::_resolve_locked(const uint32_t payload)
	{
		CDP_ASSERT(_isresolved_locked() == false && payload != std::numeric_limits<uint32_t>::max());
		//dependency must not be solved from another place

		m_payload = payload;

		//detach tasks to be processed from the call place
		auto r = m_first_task;
		m_first_task = std::numeric_limits<uint32_t>::max();
		return r;
	}

	bool 	 dependency::_isresolved_locked() const
	{
		return m_payload != std::numeric_limits<uint32_t>::max();
	}

	
}
