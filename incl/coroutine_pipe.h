#pragma once

#include "dependency.h"

namespace cdp
{

	struct coroutine_pipe : public suspend_context
	{
	public:
		void execute_frame(coroutine&& co);

		void execute_in_frame(coroutine::coroutine_list colist);
		void execute_in_queue(coroutine::coroutine_list colist);

	public:
		virtual void push_async(coroutine&& co) = 0;

	public:
		~coroutine_pipe();

	public://co_await:
		static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);
		inline bool await_ready()
		{
			return false;
		}
		inline void await_resume()
		{
		}
		inline void await_suspend(coroutine::handle_type h)
		{
			h.promise().frame_function << this;
		}
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	/*template <class F>
	inline coroutine::await_suspend_frame_function<F>::await_suspend_frame_function(F&& _func, coroutine_context& coctx)
		: func(std::move(_func))
	{
		auto frame_function = [](suspend_context& sc, coroutine& co, coroutine_pipe& pipe) -> bool {
			await_suspend_frame_function<F>& self = static_cast<await_suspend_frame_function<F>&>(sc);
			return self.func(co, pipe);
		};

		coctx.frame_function.func = frame_function;
		coctx.frame_function.context = this;
	}*/

}
