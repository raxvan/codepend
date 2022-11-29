#pragma once

#include "coroutine_handle.h"

namespace cdp
{

	struct dependency_machiene;

	struct dependency
	{
	public:
		dependency_handle_t id = cdp::invalid_dependency_handle();
		uint32_t first_task = std::numeric_limits<uint32_t>::max();

		uint64_t payload;
	public:
		void satisfy(const uint64_t value)
		{
			CDP_ASSERT(payload == 0);
			payload = value;
		}

		bool satisfied() const
		{
			return payload != 0;
		}
	};


}