#pragma once

#include "dependency.h"
#include "coroutine_dependency_pool.h"

namespace cdp
{
	struct coroutine_pipe : public threading::async_pipe<coroutine>
	{
	protected:
		coroutine_dependency_pool	     m_coroutine_pool;
	public:
		void resolve(dependency& dep);

		void run_frame(coroutine&& co);
	};
}
