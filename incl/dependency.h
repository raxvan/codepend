#pragma once

#include "coroutine_context.h"

namespace cdp
{
	struct coroutine_pipe;

	//--------------------------------------------------------------------------------------------------------------------------------
	struct dependency : public threading::spin_lock
	{
	public:
		/*
		void 	 resolve(const uint32_t payload = 0);
		void 	 resolve_in_frame(const uint32_t payload = 0);
		bool     resolved();
		*/
	public:
		~dependency();

		dependency() = default;

		dependency(const dependency&) = delete;
		dependency(dependency&&) = delete;
		dependency& operator = (const dependency&) = delete;
		dependency& operator = (dependency&&) = delete;

		bool resolved();
	protected:
		friend struct coroutine_pipe;
		friend struct coroutine_dependency_pool;
		friend struct coroutine;
		
		coroutine::handle_type resolve(const uint32_t payload);
	protected:
		bool 	 _isresolved_locked() const;
		coroutine::handle_type _resolve_locked(const uint32_t payload);

	protected:
		coroutine::handle_type 	waiting_list;

		uint32_t m_payload = std::numeric_limits<uint32_t>::max();
	};

	//--------------------------------------------------------------------------------------------------------------------------------

}
