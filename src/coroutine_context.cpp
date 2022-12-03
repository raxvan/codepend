
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

	coroutine::queue_coroutine_function::queue_coroutine_function(handle_type colist, coroutine_context& coctx)
	{
		resolve_list = colist;
		CDP_ASSERT(coctx.frame_function.valid() == false);
		if (resolve_list)
		{
			coctx.frame_function.func = queue_coroutine_function::frame_function;
			coctx.frame_function.context = this;
		}
	}
	bool coroutine::queue_coroutine_function::await_ready()
	{
		if (resolve_list)
			return false;
		return true;
	}
	bool coroutine::queue_coroutine_function::frame_function(suspend_context& sc, coroutine&, coroutine_pipe& pipe, const bool recursive)
	{
		queue_coroutine_function& dr = static_cast<queue_coroutine_function&>(sc);
		CDP_ASSERT(dr.resolve_list);

		auto h = dr.resolve_list;
		do
		{
			handle_type hnext;
			{
				auto& p = h.promise();
				hnext = p.next;
				p.next = handle_type {};
			}

			if (recursive)
			{
				coroutine co(h);
				pipe.execute_frame(co, true);
			}
			else
				pipe.push_async(coroutine(h));

			h = hnext;
		} while (h);
		return true;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	bool coroutine::dependency_await_base::await_ready()
	{
		return awaiting_dependency == nullptr;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	void coroutine::dependency_await::await_resume()
	{
#ifdef CDP_ENABLE_ASSERT
		if(awaiting_dependency != nullptr)
		{
			uint32_t tmp;
			CDP_ASSERT(awaiting_dependency->resolved(tmp) == true);
		}
#endif
	}
	coroutine::dependency_await::dependency_await(dependency& d, coroutine_context& coctx)
	{
		if (d._isresolved() == false)
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
		uint32_t tmp = std::numeric_limits<uint32_t>::max();
		if(aw.awaiting_dependency->_lock_for_await(tmp))
		{
			auto cohandle = co.detach();
			aw.awaiting_dependency->_attach(cohandle);
			aw.awaiting_dependency->_unlock_unresolved();
			return false;
		}

		return true;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	coroutine::dependency_value_await::dependency_value_await(dependency& d, coroutine_context& coctx)
	{
		if (d.resolved(result) == false)
		{
			awaiting_dependency = &d;

			CDP_ASSERT(coctx.frame_function.valid() == false);
			coctx.frame_function.func = dependency_value_await::frame_function;
			coctx.frame_function.context = this;
		}
		else
		{
			awaiting_dependency = nullptr;
		}
	}

	bool coroutine::dependency_value_await::frame_function(suspend_context& sc, coroutine& co, coroutine_pipe&, const bool)
	{
		dependency_value_await& aw = static_cast<dependency_value_await&>(sc);
		CDP_ASSERT(aw.awaiting_dependency != nullptr);
		
		if(aw.awaiting_dependency->_lock_for_await(aw.result))
		{
			auto cohandle = co.detach();
			aw.awaiting_dependency->_attach(cohandle);
			aw.awaiting_dependency->_unlock_unresolved();
			return false;
		}
		else
		{
			aw.awaiting_dependency = nullptr;
		}

		return true;
	}

	uint32_t coroutine::dependency_value_await::await_resume()
	{
		if(awaiting_dependency != nullptr)
			return awaiting_dependency->get();
		else
			return result;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	coroutine::queue_coroutine_function coroutine::coroutine_context::yield_value(dependency& d)
	{
		return coroutine::queue_coroutine_function(d.resolve(), *this);
	}
	coroutine::queue_coroutine_function coroutine::coroutine_context::yield_value(dependency* dptr)
	{
		CDP_ASSERT(dptr != nullptr);
		return coroutine::queue_coroutine_function(dptr->resolve(), *this);
	}
	coroutine::queue_coroutine_function coroutine::coroutine_context::yield_value(resolved_dependency_yield dy)
	{
		return coroutine::queue_coroutine_function(dy.resolve_list, *this);
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	coroutine::coroutine_context::~coroutine_context()
	{
		CDP_ASSERT(refcount == 0);
	}

	coroutine& coroutine::operator=(const coroutine& other)
	{
		coroutine tmp(other);
		swap(tmp);
		return (*this);
	}
	coroutine& coroutine::operator=(coroutine&& other)
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

	coroutine coroutine::operator+(cosignal& csg) const
	{
		CDP_ASSERT(handle && handle.promise().destroy_signal == nullptr);
		csg.mark();
		handle.promise().destroy_signal = &csg;

		return (*this);
	}
	coroutine coroutine::operator+(cosignal* csg) const
	{
		CDP_ASSERT(csg != nullptr);
		return (*this) + *csg;
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
					destroy_signal->arrive_and_continue();
			}
		}
	}
}
