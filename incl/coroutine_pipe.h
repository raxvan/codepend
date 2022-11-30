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
		void resolve(dependency& dep);//resolve dependency, queue coroutines to pipe
		
	public:
		uint32_t resolve_in_frame(dependency& dep);
		//^resolve rependency, run coroutines in the current frame, 
		//returns the number of coroutines that finished
		bool execute_frame(coroutine&& co);//return true if coroutine has finished

		bool pipe_empty();
		bool dependency_empty();

	public:
		using threading::async_pipe<coroutine>::push_back;
		void push_back(dependency& dep, coroutine&& co);

	public:
		~coroutine_pipe();

#ifdef CDP_TESTING
		void force_clear_dependency_tasks();
		//^ stay away from this; all coroutines waiting on dependencys are destroyed
#endif

	protected:
		template <class T>
		friend struct constref_dependency;

		static void _remove_waiting_dependency(coroutine& co);
		template <class F>
		inline void pipe_tasks_list(uint32_t task_itr, const F& _func)
		{
			while(task_itr != std::numeric_limits<uint32_t>::max())
			{
				depdency_task dtask;

				{
					m_coroutine_pool.remove_task(task_itr, dtask);
					task_itr = dtask.next;
				}
				
				_remove_waiting_dependency(dtask.task);

				_func(std::move(dtask.task));
			}
		}

	};

	template <class T>
	inline void constref_dependency<T>::resolve(const T& v)
	{
		uint32_t t;
		{
			std::lock_guard<threading::spin_lock> _(*this);
			value = v;
			t = this->_resolve_locked();
		}
		CDP_ASSERT(pipe != nullptr);
		pipe->pipe_tasks_list(t, [pipe](coroutine&& co){
			pipe->push_back(std::move(co));
		});
	}
}
