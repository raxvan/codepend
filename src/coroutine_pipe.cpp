
#include <coroutine_pipe.h>
#include <dependency.h>

namespace cdp
{

	coroutine_pipe::~coroutine_pipe()
	{
		CDP_ASSERT(pipe_empty() == true);
	}

	bool coroutine_pipe::pipe_empty()
	{
		return threading::async_pipe<coroutine>::empty();
	}

	void coroutine_pipe::push_async(coroutine&& co)
	{
		threading::async_pipe<coroutine>::push_back(std::move(co));
	}

	void coroutine_pipe::push_to(dependency& dep, coroutine&& co)
	{
		std::lock_guard<threading::spin_lock> _(dep);
		CDP_ASSERT(dep._isresolved_locked() == false); // unresolved

		auto cohandle = co.detach();

		cohandle.promise().next = dep.waiting_list;
		dep.waiting_list = cohandle;
	}

	void coroutine_pipe::resolve_recursive(dependency& d, const uint32_t payload)
	{
		auto h = d.resolve(payload);
		while (h)
		{
			coroutine::handle_type hnext;
			{
				auto& p = h.promise();
				hnext = p.next;
				p.next = coroutine::handle_type {};
			}
			coroutine co(h);
			this->execute_frame(co, true);
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

			CDP_ASSERT(!co.handle);

			return false;
		}
	}

}
