
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

	bool coroutine::await_on_resolve_impl::await_ready()
	{
		if (resolve_list)
			return false;
		return true;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	coroutine::await_on_resolve::await_on_resolve(handle_type colist, coroutine_context& coctx)
	{
		resolve_list = colist;
		//CDP_ASSERT(coctx.frame_function.valid() == false);
		if (resolve_list)
		{
			coctx.frame_function.func = await_on_resolve::frame_function;
			coctx.frame_function.context = this;
		}
	}
	
	bool coroutine::await_on_resolve::frame_function(suspend_context& sc, coroutine&, coroutine_pipe& pipe, const bool recursive)
	{
		await_on_resolve& dr = static_cast<await_on_resolve&>(sc);
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

	bool coroutine::await_on_dependency_impl::await_ready()
	{
		return dependency_ptr == nullptr;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	void coroutine::await_on_dependency::await_resume()
	{
#ifdef CDP_ENABLE_ASSERT
		if(dependency_ptr != nullptr)
		{
			uint32_t tmp;
			CDP_ASSERT(dependency_ptr->resolved(tmp) == true);
		}
#endif
	}
	coroutine::await_on_dependency_base::await_on_dependency_base(dependency& d, coroutine_context& coctx)
	{
		if (d._isresolved() == false)
		{
			dependency_ptr = &d;

			//CDP_ASSERT(coctx.frame_function.valid() == false);
			coctx.frame_function.func = await_on_dependency_base::frame_function;
			coctx.frame_function.context = this;
		}
		else
		{
			dependency_ptr = nullptr;
		}
	}
	bool coroutine::await_on_dependency_base::frame_function(suspend_context& sc, coroutine& co, coroutine_pipe&, const bool)
	{
		await_on_dependency_base& aw = static_cast<await_on_dependency_base&>(sc);
		CDP_ASSERT(aw.dependency_ptr != nullptr);
		uint32_t tmp = std::numeric_limits<uint32_t>::max();
		if(aw.dependency_ptr->_lock_for_await(tmp))
		{
			auto cohandle = co.detach();
			aw.dependency_ptr->_attach(cohandle);
			aw.dependency_ptr->_unlock_unresolved();
			return false;
		}

		return true;
	}
	//--------------------------------------------------------------------------------------------------------------------------------

	//--------------------------------------------------------------------------------------------------------------------------------
	coroutine::await_on_dependency_value::await_on_dependency_value(dependency& d, coroutine_context& coctx)
	{
		if (d.resolved(result) == false)
		{
			dependency_ptr = &d;

			//CDP_ASSERT(coctx.frame_function.valid() == false);
			coctx.frame_function.func = await_on_dependency_value::frame_function;
			coctx.frame_function.context = this;
		}
		else
		{
			dependency_ptr = nullptr;
		}
	}

	bool coroutine::await_on_dependency_value::frame_function(suspend_context& sc, coroutine& co, coroutine_pipe&, const bool)
	{
		await_on_dependency_value& aw = static_cast<await_on_dependency_value&>(sc);
		CDP_ASSERT(aw.dependency_ptr != nullptr);
		
		if(aw.dependency_ptr->_lock_for_await(aw.result))
		{
			auto cohandle = co.detach();
			aw.dependency_ptr->_attach(cohandle);
			aw.dependency_ptr->_unlock_unresolved();
			return false;
		}
		else
		{
			aw.dependency_ptr = nullptr;
		}

		return true;
	}

	uint32_t coroutine::await_on_dependency_value::await_resume()
	{
		if(dependency_ptr != nullptr)
			return dependency_ptr->get();
		else
			return result;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	coroutine::await_on_resolve coroutine::coroutine_context::yield_value(dependency& d)
	{
		return coroutine::await_on_resolve(d.resolve(), *this);
	}
	coroutine::await_on_resolve coroutine::coroutine_context::yield_value(dependency* dptr)
	{
		CDP_ASSERT(dptr != nullptr);
		return coroutine::await_on_resolve(dptr->resolve(), *this);
	}
	coroutine::await_on_resolve coroutine::coroutine_context::yield_value(resolved_dependency_colist dy)
	{
		return coroutine::await_on_resolve(dy.resolve_list, *this);
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
		csg.add_to_arrivals();
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
