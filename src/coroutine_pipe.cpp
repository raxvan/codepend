
#include <coroutine_pipe.h>

namespace cdp
{
	void coroutine_pipe::resolve(dependency& dep)
	{
		uint32_t task_itr = dep.resolve();

		while(task_itr != std::numeric_limits<uint32_t>::max())
		{
			depdency_task dtask;

			{
				m_coroutine_pool.remove_task(task_itr, dtask);
				task_itr = dtask.next;
			}

			{
				auto& task_context = dtask.task.handle.promise();

				CDP_ASSERT(task_context.waiting_for != nullptr);

				task_context.waiting_for = nullptr;
			}

			this->push_back(std::move(dtask.task));
		}
	}

	void coroutine_pipe::run_frame(coroutine&& co)
	{
		while (true)
		{
			CDP_ASSERT(co.handle);
			co.handle();

			if (co.handle.done())
			{
				coroutine tmp;
				tmp.swap(co);
				break;
			}

			auto& p = co.handle.promise();

			auto* dependency = p.waiting_for;
			TEST_ASSERT(dependency != nullptr);
			{
				std::lock_guard<threading::spin_lock> _(dependency->mutex);
				if(dependency->resolved())
				{
					p.waiting_for = nullptr;
					continue;//keep executing, the rependency got resolved in the meantime
				}
				else
				{
					m_coroutine_pool.push_task(*dependency, co);
					break;
				}
			}
		}
	}
}
