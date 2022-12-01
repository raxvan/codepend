
#include <coroutine_pipe.h>
#include <dependency.h>

namespace cdp
{

	coroutine_pipe::~coroutine_pipe()
	{
		CDP_ASSERT(pipe_empty() == true);
	}

	dependency coroutine_pipe::create_dependency()
	{
		return dependency(*this);
	}

	/*void coroutine_pipe::_remove_waiting_dependency(coroutine& co)
	{
		auto& task_context = co.handle.promise();
		CDP_ASSERT(task_context.waiting_for != nullptr);
		task_context.waiting_for = nullptr;
	}*/

	bool coroutine_pipe::pipe_empty()
	{
		return threading::async_pipe<coroutine>::empty();
	}
	

	void coroutine_pipe::push_async(coroutine&& co)
	{
		threading::async_pipe<coroutine>::push_back(std::move(co));
	}
	void coroutine_pipe::push_to(dependency& dep, coroutine&& co)
	{
		std::lock_guard<threading::spin_lock> _(dep);
		CDP_ASSERT(dep._isresolved_locked() == false);//unresolved
		co.setwaiting(dep);
		
		auto cohandle = co.detach();
				
		cohandle.promise().next = std::move(dep.waiting_list);
		dep.waiting_list = std::move(cohandle);
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
			CDP_ASSERT(dependency != nullptr && dependency->owned(this));

			{
				std::lock_guard<threading::spin_lock> _(*dependency);
				if (dependency->_isresolved_locked())
				{
					//keep executing, the dependency got resolved in the meantime
					continue;
				}

				auto cohandle = co.detach();
				
				cohandle.promise().next = std::move(dependency->waiting_list);
				dependency->waiting_list = std::move(cohandle);

				break;
			}
		}
		return false;
	}

#ifdef CDP_TESTING
	void coroutine_pipe::force_clear_dependency_tasks()
	{
	}
#endif
}
