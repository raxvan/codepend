
#include <coroutine_dependency_pool.h>
#include <dependency.h>

namespace cdp
{
	void coroutine_dependency_pool::push_task(dependency& target_dependency, coroutine& task)
	{
		std::lock_guard<threading::spin_lock> _(m_mutex);
		uint32_t tid;
		{
			tid = make_task_id();
			auto& td = m_tasks[tid];
			td.task = std::move(task);
			td.next = target_dependency.first_task;
		}
		target_dependency.first_task = tid;
	}

	void coroutine_dependency_pool::remove_task(const uint32_t id, depdency_task& out)
	{
		std::lock_guard<threading::spin_lock> _(m_mutex);
		CDP_ASSERT(id < m_tasks.size());
		m_task_free_ids.push_back(id);
		out.swap(m_tasks[id]);
	}


	uint32_t coroutine_dependency_pool::make_task_id()
	{

		if(m_task_free_ids.size() > 0)
		{
			uint32_t i = m_task_free_ids.back();
			m_task_free_ids.pop_back();
			return i;
		}
		else
		{
			std::size_t si = m_tasks.size();
			m_tasks.resize(si + 1);
			return uint32_t(si);
		}
	}



	void depdency_task::swap(depdency_task& t)
	{
		task.swap(t.task);
		std::swap(next, t.next);
	}
}
