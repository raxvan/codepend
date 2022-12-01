
#include <coroutine_pipe.h>

namespace cdp
{

	bool suspend_data::valid() const
	{
		return func != nullptr && context != nullptr;
	}
	void suspend_data::reset()
	{
		CDP_ASSERT(valid());
		//^ avoid duplicate calls to reset
		func = nullptr;
		context = nullptr;
	}
	bool suspend_data::run(coroutine& co, coroutine_pipe& pipe, const bool recursive)
	{
		CDP_ASSERT(valid());
		return (*func)(*context, co, pipe, recursive);
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	coroutine::dependency_resolve::dependency_resolve(dependency& d, coroutine_context& coctx, const uint32_t payload)
	{
		resolve_list = d.resolve(payload);

		CDP_ASSERT(coctx.frame_function.valid() == false);
		coctx.frame_function.func = dependency_resolve::frame_function;
		coctx.frame_function.context = this;
	}
	bool coroutine::dependency_resolve::await_ready()
	{
		if (resolve_list)
			return false;
		return true;
	}
	bool coroutine::dependency_resolve::frame_function(suspend_context& sc, coroutine&, coroutine_pipe& pipe, const bool recursive)
	{
		dependency_resolve& dr = static_cast<dependency_resolve&>(sc);
		CDP_ASSERT(dr.resolve_list);

		auto h = dr.resolve_list;
		do
		{
			handle_type hnext;
			{
				auto& p = h.promise();
				hnext = p.next;
				p.next = handle_type{};
			}
			
			if(recursive)
			{
				coroutine co(h);
				pipe.execute_frame(co, true);
			}
			else
				pipe.push_async(coroutine(h));

			h = hnext;
		}
		while(h);
		return true;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	bool coroutine::dependency_await::await_ready()
	{
		return awaiting_dependency == nullptr;
	}
	void coroutine::dependency_await::await_resume()
	{
		CDP_ASSERT(awaiting_dependency == nullptr || awaiting_dependency->resolved() == true);
	}
	coroutine::dependency_await::dependency_await(dependency& d, coroutine_context& coctx)
	{
		if(d.resolved() == false)
		{
			awaiting_dependency = &d;

			CDP_ASSERT(coctx.frame_function.valid() == false);
			coctx.frame_function.func = dependency_await::frame_function;
			coctx.frame_function.context = this;
		}
		else
		{
			awaiting_dependency = nullptr;			
		}
	}

	bool coroutine::dependency_await::frame_function(suspend_context& sc, coroutine& co, coroutine_pipe&, const bool)
	{
		dependency_await& aw = static_cast<dependency_await&>(sc);
		CDP_ASSERT(aw.awaiting_dependency != nullptr);

		std::lock_guard<threading::spin_lock> _(*aw.awaiting_dependency);
		if (aw.awaiting_dependency->_isresolved_locked())
		{
			//keep executing, the dependency got resolved in the meantime
			return true;
		}

		auto cohandle = co.detach();
		
		cohandle.promise().next = aw.awaiting_dependency->waiting_list;
		aw.awaiting_dependency->waiting_list = cohandle;

		return false;
	}

	//--------------------------------------------------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------------------------------------------------

	coroutine::coroutine_context::~coroutine_context()
	{
		CDP_ASSERT(refcount == 0);
	}

	coroutine& coroutine::operator = (const coroutine& other)
	{
		coroutine tmp(other);
		swap(tmp);
		return (*this);
	}
	coroutine& coroutine::operator = (coroutine&& other)
	{
		coroutine tmp;
		swap(tmp);
		swap(other);
		return (*this);
	}
	coroutine::coroutine(coroutine&& other)
	{
		swap(other);
	}

	void coroutine::swap(coroutine& other)
	{
		std::swap(handle, other.handle);
	}

	void coroutine::reset()
	{
		CDP_ASSERT(handle);
		coroutine tmp;
		swap(tmp);
	}

	//--------------------------------------------------------------------------------------------------------------------------------
	coroutine::handle_type coroutine::detach()
	{
		auto r = handle;
		handle = handle_type{};
		return r;
	}
	void coroutine::attach(const handle_type& ht)
	{
		CDP_ASSERT(!handle);
		handle = ht;
	}
	coroutine::coroutine(const handle_type& ht)
		:handle(ht)
	{
		CDP_ASSERT(handle);
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	coroutine::coroutine(const coroutine& other)
		:handle(other.handle)
	{
		if (handle)
		{
			handle.promise().refcount++;
		}
	}
	coroutine::~coroutine()
	{
		if(handle)
		{
			auto r = (--handle.promise().refcount);
			CDP_ASSERT(r >= 0);
			if(r == 0)
			{
				auto* destroy_signal = handle.promise().destroy_signal;
				handle.destroy();
				if(destroy_signal != nullptr)
					destroy_signal->arrive_and_continue();
			}
		}
	}
}
