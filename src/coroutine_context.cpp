
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
		if(d.resolved() == false)
		{
			waiting_for = &d;
			return { &d };
		}
		return {nullptr};
	}
	coroutine::dependency_await coroutine::coroutine_context::await_transform(dependency* dptr)
	{
		CDP_ASSERT(dptr != nullptr);
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
			if((--handle.promise().refcount) == 0)
			{
				handle.destroy();
			}
		}
	}
}
