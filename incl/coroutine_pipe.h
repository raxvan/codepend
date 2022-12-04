#pragma once

#include "dependency.h"

namespace cdp
{

	struct coroutine_pipe
	{
	public:
		bool execute_frame(coroutine& co, const bool recursive); // return true if coroutine has finished

		void resolve_in_frame(dependency& dep, const uint32_t payload = 0, const bool recursive = true);
		void resolve_in_queue(dependency& dep, const uint32_t payload = 0);


	public:
		//execute while not resolving dependency
		void execute_in_frame(frame& dep, const bool recursive);
		void execute_in_queue(frame& dep);
                    
	public:
		virtual void push_async(coroutine&& co) = 0;

	public:
		~coroutine_pipe();
	};

}
