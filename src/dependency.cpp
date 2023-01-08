
#include <coroutine_pipe.h>

namespace cdp
{

	inline constexpr uint32_t _unresolved_value()
	{
		return std::numeric_limits<uint32_t>::max();
	}
	inline constexpr uint32_t _locked_value()
	{
		constexpr uint32_t v1 = std::numeric_limits<uint32_t>::max();
		constexpr uint32_t v2 = v1 - 1;
		return v2;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	dependency::~dependency()
	{
		CDP_ASSERT(resolve_state.exchange(_locked_value()) != _locked_value());
		CDP_ASSERT(waiting_list == coroutine::handle_type{});
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	void dependency::add(coroutine&& co)
	{
		auto cohandle = co.detach();
		_lock_for_resolve();
		_attach(cohandle);
		_unlock_unresolved();
	}

	coroutine::coroutine_list dependency::resolve(const uint32_t payload)
	{
		_lock_for_resolve();
		auto result = _detach();
		_unlock_resolve(payload);
		return coroutine::coroutine_list(result);
	}
	coroutine::coroutine_list dependency::detach()
	{
		_lock_for_resolve();
		auto result = _detach();
		_unlock_unresolved();
		return coroutine::coroutine_list(result);
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	bool dependency::resolved(uint32_t& out)
	{
		while (true)
		{
			uint32_t r = resolve_state.load();
			if (r == _unresolved_value())
				return false;
			if (r == _locked_value())
				continue;
			out = r;
			return true;
		}
	}
	bool dependency::resolved()
	{
		while (true)
		{
			uint32_t r = resolve_state.load();
			if (r == _unresolved_value())
				return false;
			if (r == _locked_value())
				continue;
			return true;
		}
	}

	uint32_t dependency::get()
	{
		while (true)
		{
			uint32_t r = resolve_state.load(std::memory_order_relaxed);
			if (r == _locked_value())
				continue;
			CDP_ASSERT(r != _unresolved_value());
			return r;
		}
	}

	void dependency::reset()
	{
		while (true)
		{
			uint32_t pr = resolve_state.exchange(_locked_value(), std::memory_order_acquire);

			if (pr == _unresolved_value())
			{
				break;
			}
			if (pr != _locked_value())
			{
				// resolved, change to unresolved
				CDP_ASSERT(waiting_list == coroutine::handle_type{}); // must have no attached
				break;
			}

			while (true)
			{
				if (pr == _locked_value())
					pr = resolve_state.load(std::memory_order_relaxed);
				else
					break;
			}
		}

		
		resolve_state.exchange(_unresolved_value(), std::memory_order_release);
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	bool dependency::_isresolved()
	{
		while (true)
		{
			uint32_t r = resolve_state.load();
			if (r == _locked_value())
				continue;
			return r != _unresolved_value();
		}
	}

	void dependency::_attach(coroutine::handle_type h)
	{
		CDP_ASSERT(resolve_state.load() == _locked_value());
		CDP_ASSERT(h.promise().next_parallel == coroutine::handle_type {});
		h.promise().next_parallel = this->waiting_list;
		this->waiting_list = h;
	}

	coroutine::handle_type dependency::_detach()
	{
		CDP_ASSERT(resolve_state.load() == _locked_value());
		auto result = this->waiting_list;
		this->waiting_list = coroutine::handle_type {};
		return result;
	}
	void dependency::_lock_for_resolve()
	{
		while (true)
		{
			uint32_t pr = resolve_state.exchange(_locked_value(), std::memory_order_acquire);

			if (pr == _unresolved_value())
				break;

			while (true)
			{
				if (pr == _locked_value())
					pr = resolve_state.load(std::memory_order_relaxed);
				else if (pr == _unresolved_value())
					break;
				else
				{
					CDP_ASSERT(false); // dependency already resolved
				}
			}
		}
	}

	bool dependency::_lock_for_await(uint32_t& out)
	{
		while (true)
		{
			uint32_t pr = resolve_state.exchange(_locked_value());

			if (pr == _unresolved_value())
				return true;

			if (pr == _locked_value())
			{
				do
				{
					pr = resolve_state.load();
				} while (pr == _locked_value());
			}
			else
			{
				// resolved, put the value back
				out = pr;
				resolve_state.store(pr);
				return false;
			}
		}
	}

	void dependency::_unlock_resolve(const uint32_t r)
	{
		uint32_t pr = resolve_state.exchange(r, std::memory_order_release);
		CDP_ASSERT(pr == _locked_value());
	}
	void dependency::_unlock_unresolved()
	{
		uint32_t pr = resolve_state.exchange(_unresolved_value(), std::memory_order_release);
		CDP_ASSERT(pr == _locked_value());
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	bool dependency::frame_function(suspend_context& sc, coroutine& co, coroutine_pipe&)
	{
		dependency& d = static_cast<dependency&>(sc);
		uint32_t tmp = std::numeric_limits<uint32_t>::max();
		if (d._lock_for_await(tmp))
		{
			auto cohandle = co.detach();
			d._attach(cohandle);
			d._unlock_unresolved();
			return false;
		}
		return true;
	}

	//--------------------------------------------------------------------------------------------------------------------------------

	void frame::lock()
	{
		frame_state._lock_for_resolve();
	}
	void frame::unlock()
	{
		frame_state._unlock_unresolved();
	}

	void frame::add(coroutine&& co)
	{
		frame_state._attach(co.detach());
	}
	coroutine::coroutine_list frame::detach_waiting_list()
	{
		auto result = frame_state._detach();
		return coroutine::coroutine_list(result);
	}


	//--------------------------------------------------------------------------------------------------------------------------------


	void barrier_frame::acquire()
	{
		m_signal.acquire();	
	}

	coroutine::coroutine_list barrier_frame::wait()
	{
		m_signal.wait();
		std::lock_guard<cdp::frame> _(m_frame);
		return m_frame.detach_waiting_list();
	}

	bool barrier_frame::frame_function(suspend_context& sc, coroutine& co, coroutine_pipe&)
	{
		barrier_frame& bf = static_cast<barrier_frame&>(sc);

		std::lock_guard<cdp::frame> _(bf.m_frame);
		bf.m_frame.add(std::move(co));
		bf.m_signal.release_and_continue();
		return false;
	}

	
}
