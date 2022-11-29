#pragma once

#include "codepend_config.h"

namespace cdp
{
	
	/*
	struct dependency_payload
	{
	public:
		virtual ~dependency_payload() = default;
	};
	*/

	struct dependency
	{
	public:
		threading::spin_lock mutex;
		std::atomic<bool> state{ false };

		uint32_t first_task = std::numeric_limits<uint32_t>::max();
		dependency_handle_t id = cdp::invalid_dependency_handle();
	public:
		uint32_t resolve();
		bool     resolved() const;
	};


}
