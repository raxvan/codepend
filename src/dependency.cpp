
#include <coroutine_pipe.h>

namespace cdp
{

	dependency::~dependency()
	{
		std::lock_guard<threading::spin_lock> _(*this);
		CDP_ASSERT(!waiting_list);
	}

	coroutine::handle_type dependency::resolve(const uint32_t payload)
	{
		std::lock_guard<threading::spin_lock> _(*this);
		return _resolve_locked(payload);
	}
	coroutine::handle_type dependency::_resolve_locked(const uint32_t payload)
	{
		CDP_ASSERT(this->_isresolved_locked() == false && payload != std::numeric_limits<uint32_t>::max());
		m_payload = payload;
		auto result = this->waiting_list;
		this->waiting_list = coroutine::handle_type {};
		return result;
	}
	bool dependency::resolved()
	{
		std::lock_guard<threading::spin_lock> _(*this);
		return _isresolved_locked();
	}

	bool dependency::_isresolved_locked() const
	{
		return m_payload != std::numeric_limits<uint32_t>::max();
	}

}
