#pragma once

#include "dependency.h"

namespace cdp
{
	struct coroutine_pipe : public threading::async_pipe<coroutine>
	{
	public:
		bool execute_frame(coroutine& co, const bool recursive); // return true if coroutine has finished

		void resolve_recursive(dependency& dep, const uint32_t payload = 0);

	public:
		bool pipe_empty();
		bool dependency_empty();

	public:
		void push_async(coroutine&& co);
		void push_to(dependency& dep, coroutine&& co);

	public:
		~coroutine_pipe();

	protected:
		using threading::async_pipe<coroutine>::push_back;
		friend struct dependency;
	};

}
