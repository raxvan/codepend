#pragma once

#include "coroutine_handle.h"

namespace cdp
{
	
	struct dependency_payload
	{
	public:
		virtual ~dependency_payload() = default;
	};

	struct dependency
	{
	public:
		dependency_handle_t id = cdp::invalid_dependency_handle();
		uint32_t first_task = std::numeric_limits<uint32_t>::max();

		union
		{
			uint64_t 			payload_id;
			dependency_payload* payload_ptr;
		};
	public:
		dependency()
		{
			payload_id = 0;
			payload_ptr = nullptr;
		}

		void satisfy(const uint64_t value)
		{
			CDP_ASSERT(payload_id == 0);
			payload_id = value;
		}
		void satisfy(dependency_payload* pload)
		{
			CDP_ASSERT(payload_ptr == nullptr);
			payload_ptr = pload;
		}

	public:
		bool satisfied() const
		{
			return payload_id != 0;
		}

	public:
		//co:
		bool await_ready()
		{
			return this->satisfied();
		}
		void await_suspend(std::coroutine_handle<coroutine::coroutine_context> h)
		{
			CDP_ASSERT(h.promise().waiting_for == nullptr);
			h.promise().waiting_for = this;
		}
		void await_resume()
		{
		}

	};


}