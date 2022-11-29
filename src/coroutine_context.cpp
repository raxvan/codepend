
#include <coroutine_context.h>
#include <dependency.h>

namespace cdp
{

	bool coroutine::dependency_await::await_ready()
	{
		CDP_ASSERT(waiting_for != nullptr);
		return waiting_for->resolved();
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
