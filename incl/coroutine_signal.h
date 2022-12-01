
#pragma once

#include "codepend_config.h"

namespace cdp
{

	struct cosignal
	{
	public:
		
	public:
		void wait(const uint32_t count);

	protected:
		friend struct coroutine;

		void arrive_and_continue();
	};
}