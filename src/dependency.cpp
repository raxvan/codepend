
#include <dependency.h>

namespace cdp
{
	uint32_t dependency::resolve()
	{
		std::lock_guard<threading::spin_lock> _(mutex);
		
		CDP_ASSERT(resolved_state == 0);
		//dependency must not be solved from another place

		resolved_state++;

		//detach tasks to be processed from the call place
		auto r = first_task;
		first_task = std::numeric_limits<uint32_t>::max();
		return r;
	}

	bool     dependency::resolved()
	{
		std::lock_guard<threading::spin_lock> _(mutex);
		CDP_ASSERT(resolved_state != std::numeric_limits<uint32_t>::max());
		//^ resolved but mutated into an invalid state, all waiting coroutines on this dependency must be informed

		return resolved_state != 0;
	}
}
