

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

		bool empty();


	public:
#ifdef CDP_TESTING
		void clear_for_testing();//DANGEROUS!
#endif
	protected:
		threading::spin_lock 		m_mutex;
		std::vector<uint32_t> 		m_task_free_ids;
		std::vector<depdency_task> 	m_tasks;
	};

}
