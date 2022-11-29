#pragma once

#include "dependency.h"
#include "coroutine_dependency_pool.h"

namespace cdp
{
	struct coroutine_pipe
	{
	protected:
		threading::async_pipe<coroutine> pipe;
		coroutine_dependency_pool	     dmp;
	public:
		void resolve(dependency& dep);
	public:
		/*void run(coroutine& co)
		{
			while (true)
			{
				co.handle();

				if (co.handle.done())
				{
					coroutine tmp;
					tmp.swap(co);
				}

				auto* dependency = co.handle.promise().waiting_for;
				TEST_ASSERT(dependency != nullptr);
				dependency->lock();
				DEPENDENCY_POOL.push_task(*dependency, std::move(co));
				dependency->unlock();
			}
		}*/
	};
}
