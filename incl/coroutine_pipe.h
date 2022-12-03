#pragma once

#include "dependency.h"

namespace cdp
{

	struct coroutine_pipe
	{
	public:
		bool execute_frame(coroutine& co, const bool recursive); // return true if coroutine has finished

		void resolve_recursive(dependency& dep, const uint32_t payload = 0);
		void resolve(dependency& dep, const uint32_t payload = 0);

	public:
		virtual void push_async(coroutine&& co) = 0;

	public:
		~coroutine_pipe();
	};

}
