
#include <dependency.h>

namespace cdp
{
	uint32_t dependency::resolve()
	{
		bool sr = state.exchange(true, std::memory_order_acquire);
		CDP_ASSERT(sr == false);
		auto r = first_task;
		first_task = std::numeric_limits<uint32_t>::max();
		return r;
	}

	bool     dependency::resolved() const
	{
		return state.load();
	}
}
