

#pragma once

#include "coroutine_context.h"

namespace cdp
{
	struct dependency;

	struct depdency_task
	{
		coroutine 	task;
		uint32_t 	next = std::numeric_limits<uint32_t>::max();

		void swap(depdency_task& t);
	};

	struct coroutine_dependency_pool
	{
	public:
		void push_task(dependency& target_dependency, coroutine& task);
		uint32_t make_task_id();

		void remove_task(const uint32_t id, depdency_task& out);
	protected:
		threading::spin_lock 		m_mutex;
		std::vector<uint32_t> 		m_task_free_ids;
		std::vector<depdency_task> 	m_tasks;
	};



	/*dependency_handle_t add_dependency(dependency* d)
	{
		CDP_ASSERT(d !=  nullptr);
		if(m_dependency_free_ids.size() > 0)
		{
			dependency_handle_t i = m_dependency_free_ids.back();
			m_dependency_free_ids.pop_back();
			m_dependency_list[i] = d;
			d->id = i;
			return i;
		}
		else
		{
			std::size_t si = m_dependency_list.size();
			m_dependency_list.resize(si + 1);
			m_dependency_list[si] = d;
			d->id = dependency_handle_t(si);
			return dependency_handle_t(si);
		}
	}
	dependency* remove_dependency(dependency_handle_t id)
	{
		m_dependency_free_ids.push_back(id);
		auto* r = m_dependency_list[id];
		CDP_ASSERT(r != nullptr);
		m_dependency_list[id] = nullptr;
		return r;
	}
	dependency* remove_dependency(dependency* dptr)
	{
		CDP_ASSERT(dptr != nullptr);
		return remove_dependency(dptr->id);
	}

	dependency* get_dependency(dependency_handle_t id)
	{
		auto* r = m_dependency_list[id];
		CDP_ASSERT(r != nullptr);
		return r;
	}*/


}
