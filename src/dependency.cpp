
#include <dependency.h>

namespace cdp
{
	uint32_t dependency::resolve()
	{
		std::lock_guard<threading::spin_lock> _(*this);
		return _resolve_locked();
	}

	uint32_t dependency::_resolve_locked()
	{
		CDP_ASSERT(resolved_state == 0);
		//dependency must not be solved from another place

		resolved_state++;

		//detach tasks to be processed from the call place
		auto r = first_task;
		first_task = std::numeric_limits<uint32_t>::max();
		return r;
	}

	bool 	 dependency::_resolved_locked() const
	{
		return resolved_state != 0;
	}

	bool     dependency::resolved()
	{
		std::lock_guard<threading::spin_lock> _(*this);
		
		return resolved_state != 0;
	}
}
