
#include <coroutine_pipe.h>
#include <dependency.h>

namespace cdp
{

	coroutine_pipe::~coroutine_pipe()
	{
	}

	void coroutine_pipe::_execute_list_in_frame(coroutine::handle_type h, const bool recursive)
	{
		while (h)
		{
			coroutine::handle_type hnext = h.promise().detach_parallel();
			coroutine co(h);
			this->execute_frame(co, recursive);
			h = hnext;
		}
	}
	void coroutine_pipe::_push_list_in_queue(coroutine::handle_type h)
	{
		while (h)
		{
			coroutine::handle_type hnext = h.promise().detach_parallel();
			coroutine co(h);
			this->push_async(std::move(co));
			h = hnext;
		}
	}

	bool coroutine_pipe::execute_frame(coroutine& co, const bool recursive)
	{
		auto h = co.handle;
		while (true)
		{
			h();

			if (h.done())
			{
				{
					auto hn = co.handle.promise().detach_sequential();
					if(hn)
					{
						coroutine nextco(hn);
						if(recursive)
							this->execute_frame(nextco, true);
						else
							this->push_async(std::move(nextco));
					}
				}

				co.reset();
				return true;
			}

			auto ffunc = h.promise().frame_function;

			if (ffunc.run(co, *this, recursive))
				continue;

			CDP_ASSERT(!co.handle); //it must be detached

			return false;
		}
	}

	void coroutine_pipe::execute_in_frame(frame& f, const bool recursive)
	{
		_execute_list_in_frame(f.frame_state.detach(), recursive);
	}
	void coroutine_pipe::execute_in_queue(frame& f)
	{
		_push_list_in_queue(f.frame_state.detach());
	}
	void coroutine_pipe::resolve_in_frame(dependency& d, const uint32_t payload, const bool recursive)
	{
		_execute_list_in_frame(d.resolve(payload), recursive);
	}
	void coroutine_pipe::resolve_in_queue(dependency& d, const uint32_t payload)
	{
		_push_list_in_queue(d.resolve(payload));
	}

}
