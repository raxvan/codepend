
#include <coroutine_pipe.h>

namespace cdp
{
	void resolve(dependency& dep)
	{
		uint32_t task_itr = dep.resolve();

		while(task_itr != std::numeric_limits<uint32_t>::max())
		{
			depdency_task dtask;

			{
				dmp.remove_task(task_itr, dtask);
				task_itr = dtask.next;
			}

			{
				auto& task_context = dtask.task.handle.promise();

				CDP_ASSERT(task_context.waiting_for != nullptr);

				task_context.waiting_for = nullptr;
			}

			pipe.push_back(std::move(dtask.task));
		}

		dep.unlock();
	}
}
