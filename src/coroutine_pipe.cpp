
#include <coroutine_pipe.h>
#include <dependency.h>

namespace cdp
{

	coroutine_pipe::~coroutine_pipe()
	{
	}

	void coroutine_pipe::execute_in_frame(coroutine::coroutine_list h)
	{
		auto itr = h.first;
		while (itr)
		{
			coroutine::handle_type hnext = itr.promise().detach_parallel();
			this->execute_frame(coroutine(itr));
			itr = hnext;
		}
	}
	void coroutine_pipe::execute_in_queue(coroutine::coroutine_list h)
	{
		auto itr = h.first;
		while (itr)
		{
			coroutine::handle_type hnext = itr.promise().detach_parallel();
			this->push_async(coroutine(itr));
			itr = hnext;
		}
	}

	void coroutine_pipe::execute_frame(coroutine&& co)
	{
		auto h = co.handle;
		while (true)
		{
			h();

			if (h.done())
			{
				auto hn = h.promise().detach_sequential();
				co.reset();

				if (hn)
					execute_frame(coroutine(hn));

				break;
			}

			auto ffunc = h.promise().frame_function;

			if (ffunc.run(co, *this))
				continue;

			break;
		}
		CDP_ASSERT(!co.handle); // it must be destroyed or attached to something
	}

}
