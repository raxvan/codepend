

#pragma once

#include "dependency.h"

#include <vector>

namespace cdp
{

	struct dependency_machine
	{
	public:
		
		template <class TASK_PIPE, class PL >
		void resolve(TASK_PIPE& pipe, dependency& dep, const PL& payload)
		{
			auto itr = dep.first_task;

			dep.first_task = std::numeric_limits<uint32_t>::max();
			dep.satisfy(payload);

			while(itr != std::numeric_limits<uint32_t>::max())
			{
				auto& t = m_tasks[itr];

				CDP_ASSERT(t.task.handle.promise().waiting_for != nullptr);
				t.task.handle.promise().waiting_for = nullptr;

				pipe.push_back(std::move(t.task));

				auto next = t.next;
				t.next = std::numeric_limits<uint32_t>::max();

				m_task_free_ids.push_back(itr);

				itr = next;
			}
		}

	public:
		dependency_handle_t add_dependency(dependency* d)
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
		}

	public:
		struct depdency_task
		{
			coroutine 	task;
			uint32_t 	next = std::numeric_limits<uint32_t>::max();
		};
		void push_task(dependency& dep, coroutine task)
		{
			uint32_t t = create_task();
			auto& td = m_tasks[t];
			td.task = std::move(task);
			td.next = dep.first_task;

			dep.first_task = t;
		}

	protected:
		std::vector<uint32_t> 		m_dependency_free_ids;
		std::vector<dependency*> 	m_dependency_list;


		std::vector<uint32_t> 		m_task_free_ids;
		std::vector<depdency_task> 	m_tasks;


	protected:
		uint32_t create_task()
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

	protected:
		

	};

}
