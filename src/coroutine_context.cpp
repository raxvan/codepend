
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


	coroutine::await_move_to_queue::await_move_to_queue(coroutine_pipe& p, coroutine_context& coctx)
		:pipe(p)
	{
		coctx.frame_function.func = await_move_to_queue::frame_function;
		coctx.frame_function.context = this;
	}
	bool coroutine::await_move_to_queue::frame_function(suspend_context& sc, coroutine& co, coroutine_pipe&)
	{
		await_move_to_queue& dr = static_cast<await_move_to_queue&>(sc);
		dr.pipe.push_async(std::move(co));	
		return false;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	bool coroutine::await_suspend_with_colist::await_ready()
	{
		return (list.first == handle_type{});
	}

	coroutine::await_suspend_with_colist::await_suspend_with_colist(handle_type first, coroutine_context& coctx)
	{
		list.first = first;
		if (list.first)
		{
			coctx.frame_function.func = await_suspend_with_colist::frame_function;
			coctx.frame_function.context = this;
		}
	}

	bool coroutine::await_suspend_with_colist::frame_function(suspend_context& sc, coroutine&, coroutine_pipe& pipe)
	{
		await_suspend_with_colist& dr = static_cast<await_suspend_with_colist&>(sc);
		CDP_ASSERT(dr.list.first);
		pipe.execute_in_queue(dr.list);
		return true;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	bool coroutine::await_on_dependency::await_ready()
	{
		return dependency_ptr == nullptr;
	}
	void coroutine::await_on_dependency::await_resume()
	{
		CDP_ASSERT(dependency_ptr == nullptr || dependency_ptr->_isresolved() == true);
	}
	coroutine::await_on_dependency::await_on_dependency(dependency& d, coroutine_context& coctx)
	{
		if (d._isresolved() == false)
		{
			dependency_ptr = &d;

			// CDP_ASSERT(coctx.frame_function.valid() == false);
			coctx.frame_function.func = await_on_dependency::frame_function;
			coctx.frame_function.context = this;
		}
		else
		{
			dependency_ptr = nullptr;
		}
	}
	bool coroutine::await_on_dependency::frame_function(suspend_context& sc, coroutine& co, coroutine_pipe&)
	{
		await_on_dependency& aw = static_cast<await_on_dependency&>(sc);
		CDP_ASSERT(aw.dependency_ptr != nullptr);
		uint32_t tmp = std::numeric_limits<uint32_t>::max();
		if (aw.dependency_ptr->_lock_for_await(tmp))
		{
			auto cohandle = co.detach();
			aw.dependency_ptr->_attach(cohandle);
			aw.dependency_ptr->_unlock_unresolved();
			return false;
		}

		return true;
	}
	//--------------------------------------------------------------------------------------------------------------------------------
	coroutine::await_on_dependency_value::await_on_dependency_value(dependency& d, coroutine_context& coctx)
	{
		if (d.resolved(result) == false)
		{
			dependency_ptr = &d;

			// CDP_ASSERT(coctx.frame_function.valid() == false);
			coctx.frame_function.func = await_on_dependency_value::frame_function;
			coctx.frame_function.context = this;
		}
		else
		{
			dependency_ptr = nullptr;
		}
	}

	bool coroutine::await_on_dependency_value::frame_function(suspend_context& sc, coroutine& co, coroutine_pipe&)
	{
		await_on_dependency_value& aw = static_cast<await_on_dependency_value&>(sc);
		CDP_ASSERT(aw.dependency_ptr != nullptr);

		if (aw.dependency_ptr->_lock_for_await(aw.result))
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
	bool coroutine::await_on_dependency_value::await_ready()
	{
		return dependency_ptr == nullptr;
	}

	uint32_t coroutine::await_on_dependency_value::await_resume()
	{
		if (dependency_ptr != nullptr)
			return dependency_ptr->get();
		else
			return result;
	}

	//--------------------------------------------------------------------------------------------------------------------------------
	
	coroutine::await_on_frame::await_on_frame(frame& f, coroutine_context& coctx)
		: frame_ptr(&f)
	{

		coctx.frame_function.func = await_on_frame::frame_function;
		coctx.frame_function.context = this;
	}
	bool coroutine::await_on_frame::frame_function(suspend_context& sc, coroutine& co, coroutine_pipe&)
	{
		await_on_frame& aw = static_cast<await_on_frame&>(sc);
		CDP_ASSERT(aw.frame_ptr != nullptr);
		auto& f = *aw.frame_ptr;

		auto cohandle = co.detach();
		f.add(cohandle);
		return false;
	}

	//--------------------------------------------------------------------------------------------------------------------------------
	coroutine::await_suspend_with_colist coroutine::coroutine_context::await_transform(coroutine_list colist)
	{
		return await_suspend_with_colist(colist.first, *this);
	}
	
	coroutine::await_move_to_queue coroutine::coroutine_context::await_transform(coroutine_pipe& p)
	{
		return coroutine::await_move_to_queue(p, *this);
	}

	coroutine::await_on_dependency coroutine::coroutine_context::await_transform(dependency& d)
	{
		return await_on_dependency(d, *this);
	}
	coroutine::await_on_dependency coroutine::coroutine_context::await_transform(dependency* dptr)
	{
		CDP_ASSERT(dptr != nullptr);
		return await_on_dependency(*dptr, *this);
	}
	coroutine::await_on_frame coroutine::coroutine_context::await_transform(frame& f)
	{
		return await_on_frame(f, *this);
	}
	coroutine::await_on_frame coroutine::coroutine_context::await_transform(frame* fptr)
	{
		CDP_ASSERT(fptr != nullptr);
		return await_on_frame(*fptr, *this);
	}
	coroutine::await_on_dependency_value coroutine::coroutine_context::await_transform(result<uint32_t>& d)
	{
		return coroutine::await_on_dependency_value(d, *this);
	}
	coroutine::await_on_dependency_value coroutine::coroutine_context::await_transform(result<uint32_t>* dptr)
	{
		CDP_ASSERT(dptr != nullptr);
		return coroutine::await_on_dependency_value(*dptr, *this);
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
