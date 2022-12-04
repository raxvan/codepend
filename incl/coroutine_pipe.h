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


	//--------------------------------------------------------------------------------------------------------------------------------

	template <class F>
	inline coroutine::await_coroutine_generator<F>::await_coroutine_generator(F&& _func, coroutine_context& coctx)
		:func(std::move(_func))
	{
		auto frame_function = [](suspend_context& sc, coroutine&, coroutine_pipe& pipe, const bool recursive) -> bool
		{
			await_coroutine_generator<F>& self = static_cast<await_coroutine_generator<F>&>(sc);
			auto new_coroutine = self.func();
			if (recursive)
				pipe.execute_frame(new_coroutine, true);
			else
				pipe.push_async(std::move(new_coroutine));

			return true;
		};

		coctx.frame_function.func = frame_function;
		coctx.frame_function.context = this;
	}

}
