
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
	bool suspend_data::run(coroutine& co, coroutine_pipe& pipe)
	{
		CDP_ASSERT(valid());
		return (*func)(*context, co, pipe);
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	bool coroutine::coroutine_list::frame_function(suspend_context& sc, coroutine&, coroutine_pipe& pipe)
	{
		coroutine::coroutine_list& d = static_cast<coroutine::coroutine_list&>(sc);
		pipe.execute_in_queue(d);
		return true;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	coroutine::coroutine_context::~coroutine_context()
	{
		CDP_ASSERT(refcount == 0 && next_parallel == handle_type {} && next_sequential == handle_type {});
	}

	coroutine::handle_type coroutine::coroutine_context::detach_parallel()
	{
		auto r = next_parallel;
		next_parallel = handle_type {};
		return r;
	}
	void coroutine::coroutine_context::add_parallel(coroutine::handle_type h)
	{
		CDP_ASSERT(h.promise().next_parallel == handle_type {});
		h.promise().next_parallel = next_parallel;
		next_parallel = h;
	}

	coroutine::handle_type coroutine::coroutine_context::detach_sequential()
	{
		auto r = next_sequential;
		next_sequential = handle_type {};
		return r;
	}
	void coroutine::coroutine_context::add_sequential(coroutine::handle_type h)
	{
		CDP_ASSERT(h.promise().next_sequential == handle_type {});
		h.promise().next_sequential = next_sequential;
		next_sequential = h;
	}

	void* coroutine::coroutine_context::operator new(size_t size)
	{
		return ::operator new(size);
	}
	void coroutine::coroutine_context::operator delete(void* p)
	{
		::delete (p);
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	coroutine& coroutine::operator=(const coroutine& other)
	{
		coroutine tmp(other);
		swap(tmp);
		return (*this);
	}
	coroutine& coroutine::operator=(coroutine&& other) noexcept
	{
		coroutine tmp;
		swap(tmp);
		swap(other);
		return (*this);
	}
	coroutine::coroutine(coroutine&& other) noexcept
	{
		swap(other);
	}
	bool coroutine::valid() const
	{
		return handle != handle_type{};
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

	void coroutine::set_signal(cosignal& csg)
	{
		CDP_ASSERT(handle && handle.promise().destroy_signal == nullptr);
		csg.acquire();
		handle.promise().destroy_signal = &csg;
	}
	void coroutine::set_signal(cosignal* csg)
	{
		CDP_ASSERT(csg != nullptr);
		set_signal(*csg);
	}

	//--------------------------------------------------------------------------------------------------------------------------------
	
	coroutine::handle_type coroutine::detach()
	{
		auto r = handle;
		handle = handle_type {};
		return r;
	}
	void coroutine::attach(const handle_type& ht)
	{
		CDP_ASSERT(!handle);
		handle = ht;
	}
	coroutine::coroutine(const handle_type& ht)
		: handle(ht)
	{
		CDP_ASSERT(handle);
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	coroutine::coroutine(const coroutine& other)
		: handle(other.handle)
	{
		if (handle)
		{
			handle.promise().refcount++;
		}
	}
	coroutine::~coroutine()
	{
		if (handle)
		{
			auto r = (--handle.promise().refcount);
			CDP_ASSERT(r >= 0);
			if (r == 0)
			{
				auto* destroy_signal = handle.promise().destroy_signal;
				handle.destroy();
				if (destroy_signal != nullptr)
					destroy_signal->release_and_continue();
			}
		}
	}
}
