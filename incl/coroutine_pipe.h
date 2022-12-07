#pragma once

#include "dependency.h"

namespace cdp
{

	struct coroutine_pipe
	{
	public:
		void execute_frame(coroutine&& co); // return true if coroutine has finished

	public:
		void resolve_in_frame(dependency& dep, const uint32_t payload = 0);
		void resolve_in_queue(dependency& dep, const uint32_t payload = 0);

		void execute_in_frame(frame& dep);
		void execute_in_queue(frame& dep);

	public:
		virtual void push_async(coroutine&& co) = 0;

		void execute_list_in_frame(coroutine::handle_type h);
		void push_list_in_queue(coroutine::handle_type h);

	protected:
		friend struct coroutine;
		
		

	public:
		~coroutine_pipe();
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	template <class F>
	inline coroutine::await_on_frame_function<F>::await_on_frame_function(F&& _func, coroutine_context& coctx)
		: func(std::move(_func))
	{
		auto frame_function = [](suspend_context& sc, coroutine& co, coroutine_pipe& pipe) -> bool {
			await_on_frame_function<F>& self = static_cast<await_on_frame_function<F>&>(sc);
			return self.func(co, pipe);
		};

		coctx.frame_function.func = frame_function;
		coctx.frame_function.context = this;
	}

}
