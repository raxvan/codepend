
#include <coroutine_context.h>
#include <dependency.h>

namespace cdp
{

	bool coroutine::dependency_await::await_ready()
	{
		return awaiting_dependency == nullptr;
	}
	void coroutine::dependency_await::await_resume()
	{
		CDP_ASSERT(awaiting_dependency == nullptr || awaiting_dependency->resolved() == true);
	}

	coroutine::dependency_await coroutine::coroutine_context::await_transform(dependency& d)
	{
		//we check here if it's resolved or not so we can later assert if we waited on it
		CDP_ASSERT(waiting_for == nullptr);
		if(d.resolved() == false)
		{
			waiting_for = &d;
			return { &d };
		}
		return {nullptr};
	}
	coroutine::dependency_await coroutine::coroutine_context::await_transform(dependency* dptr)
	{
		CDP_ASSERT(dptr != nullptr && waiting_for == nullptr);
		if(dptr->resolved() == false)
		{
			waiting_for = dptr;
			return { dptr };
		}
		return {nullptr};
	}

	coroutine::coroutine_context::~coroutine_context()
	{
		CDP_ASSERT(refcount == 0 && waiting_for == nullptr);
	}

	//--------------------------------------------------------------------------------------------------------------------------------
	/*
	coroutine& coroutine::operator >> (threading::barrier& b)
	{
		CDP_ASSERT(handle);
		CDP_ASSERT(handle.promise().destroy_signal == nullptr);
		handle.promise().destroy_signal = &b;
		return (*this);
	}
	*/

	bool coroutine::iswaiting() const
	{
		CDP_ASSERT(handle);
		return handle.promise().waiting_for != nullptr;
	}
	void coroutine::setwaiting(dependency& d)
	{
		CDP_ASSERT(handle && handle.promise().waiting_for == nullptr);
		handle.promise().waiting_for = &d;
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

	coroutine::coroutine(handle_type ht)
		:handle(std::move(ht))
	{
		handle.promise().refcount++;
	}
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
