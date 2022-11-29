
#include <dependency.h>

namespace cdp
{
	uint32_t dependency::resolve()
	{
		std::lock_guard<threading::spin_lock> _(mutex);
		CDP_ASSERT(resolved() == false);
		state.store(true);

		//detach tasks
		auto r = first_task;
		first_task = std::numeric_limits<uint32_t>::max();
		return r;
	}

	bool     dependency::resolved() const
	{
		return state.load();
	}
}
