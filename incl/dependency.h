#pragma once

#include "codepend_config.h"

namespace cdp
{

	struct dependency
	{
	protected:
		friend struct coroutine_pipe;
		friend struct coroutine_dependency_pool;
		
		threading::spin_lock mutex;
		
		uint32_t resolved_state = 0;

		uint32_t first_task = std::numeric_limits<uint32_t>::max();
	public:
		uint32_t resolve();
		bool     resolved();

	private:

	};


}
