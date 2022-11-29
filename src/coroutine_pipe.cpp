
#include <coroutine_pipe.h>

namespace cdp
{

	coroutine_pipe::~coroutine_pipe()
	{
		CDP_ASSERT(pipe_empty() == true);
		CDP_ASSERT(dependency_empty() == true);
	}

	void coroutine_pipe::_remove_waiting_dependency(coroutine& co)
	{
		auto& task_context = co.handle.promise();
		CDP_ASSERT(task_context.waiting_for != nullptr);
		task_context.waiting_for = nullptr;
	}
	void coroutine_pipe::resolve(dependency& dep)
	{
		pipe_tasks_list(dep.resolve(), [this](coroutine&& co){
			this->push_back(std::move(co));
		});
	}
	void coroutine_pipe::resolve_in_frame(dependency& dep)
	{
		pipe_tasks_list(dep.resolve(), [this](coroutine&& co){
			this->run_frame(std::move(co));
		});
	}

	bool coroutine_pipe::pipe_empty()
	{
		return threading::async_pipe<coroutine>::empty();
	}
	bool coroutine_pipe::dependency_empty()
	{
		return m_coroutine_pool.empty();
	}

	void coroutine_pipe::run_frame(coroutine&& co)
	{
		CDP_ASSERT(co.handle);
		while (true)
		{
			co.handle();

			if (co.handle.done())
			{
				coroutine tmp;
				tmp.swap(co);
				break;
			}

			auto* dependency = co.handle.promise().waiting_for;

			CDP_ASSERT(dependency != nullptr);

			{
				std::lock_guard<threading::spin_lock> _(dependency->mutex);
				if(dependency->resolved_state != 0)
					continue;//keep executing, the dependency got resolved in the meantime

				m_coroutine_pool.push_task(*dependency, co);
				break;
			}
		}
	}

#ifdef CDP_TESTING
	void coroutine_pipe::force_clear_dependency_tasks()
	{
		m_coroutine_pool.clear_for_testing();
	}
#endif
}
