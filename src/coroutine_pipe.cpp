
#include <coroutine_pipe.h>
#include <dependency.h>

namespace cdp
{

	coroutine_pipe::~coroutine_pipe()
	{
	}

	inline coroutine::handle_type pop_next(coroutine::handle_type h)
	{
		coroutine::handle_type hnext;
		{
			auto& p = h.promise();
			hnext = p.next;
			p.next = coroutine::handle_type {};
		}
		return hnext;
	}

	void coroutine_pipe::execute_in_frame(frame& f, const bool recursive)
	{
		auto h = f.frame_state.detach();
		while (h)
		{
			coroutine::handle_type hnext = pop_next(h);
			coroutine co(h);
			this->execute_frame(co, recursive);
			h = hnext;
		}
	}
	void coroutine_pipe::execute_in_queue(frame& f)
	{
		auto h = f.frame_state.detach();
		while (h)
		{
			coroutine::handle_type hnext = pop_next(h);
			coroutine co(h);
			this->push_async(std::move(co));
			h = hnext;
		}
	}

	void coroutine_pipe::resolve_in_frame(dependency& d, const uint32_t payload, const bool recursive)
	{
		auto h = d.resolve(payload);
		while (h)
		{
			coroutine::handle_type hnext = pop_next(h);
			coroutine co(h);
			this->execute_frame(co, recursive);
			h = hnext;
		}
	}
	void coroutine_pipe::resolve_in_queue(dependency& d, const uint32_t payload)
	{
		auto h = d.resolve(payload);
		while (h)
		{
			coroutine::handle_type hnext = pop_next(h);
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

}
