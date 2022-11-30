
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
	uint32_t coroutine_pipe::resolve_in_frame(dependency& dep)
	{
		uint32_t r = 0;
		pipe_tasks_list(dep.resolve(), [this, &r](coroutine&& co){
			if (this->execute_frame(std::move(co)))
				r++;
		});
		return r;
	}

	bool coroutine_pipe::pipe_empty()
	{
		return threading::async_pipe<coroutine>::empty();
	}
	bool coroutine_pipe::dependency_empty()
	{
		return m_coroutine_pool.empty();
	}

	void coroutine_pipe::push_back(dependency& dep, coroutine&& co)
	{
		std::lock_guard<threading::spin_lock> _(dep);
		CDP_ASSERT(dep.resolved_state == 0);//unresolved
		CDP_ASSERT(co.handle.promise().waiting_for == nullptr);
		co.handle.promise().waiting_for = &dep;
		m_coroutine_pool.push_task(dep, std::move(co));
	}

	bool coroutine_pipe::execute_frame(coroutine&& co)
	{
		CDP_ASSERT(co.handle);
		while (true)
		{
			co.handle();

			if (co.handle.done())
			{
				coroutine tmp;
				tmp.swap(co);
				return true;
			}

			auto* dependency = co.handle.promise().waiting_for;
			CDP_ASSERT(dependency != nullptr);

			{
				std::lock_guard<threading::spin_lock> _(*dependency);
				if (dependency->resolved_state != 0)
				{
					continue;//keep executing, the dependency got resolved in the meantime
				}

				m_coroutine_pool.push_task(*dependency, std::move(co));
				break;
			}
		}
		return false;
	}

#ifdef CDP_TESTING
	void coroutine_pipe::force_clear_dependency_tasks()
	{
		m_coroutine_pool.clear_for_testing();
	}
#endif
}
