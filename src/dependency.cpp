
#include <coroutine_pipe.h>

namespace cdp
{

	inline constexpr uint32_t      _unresolved_value()
	{
		return std::numeric_limits<uint32_t>::max();
	}
	inline constexpr uint32_t      _locked_value()
	{
		constexpr uint32_t v1 = std::numeric_limits<uint32_t>::max();
		constexpr uint32_t v2 = v1 - 1;
		return v2;
	}


	dependency::~dependency()
	{
	}

	void dependency::add(coroutine&& co)
	{
		auto cohandle = co.detach();
		_lock_for_resolve();
		_attach(cohandle);
		_unlock_unresolved();
	}

	coroutine::handle_type dependency::resolve(const uint32_t payload)
	{
		_lock_for_resolve();
		auto result = _detach();
		_resolve(payload);
		return result;
	}
	bool dependency::resolved(uint32_t& out)
	{
		while(true)
		{
			uint32_t r = resolve_state.load();
			if(r == _unresolved_value())
				return false;
			if(r == _locked_value())
				continue;
			out = r;
			return true;
		}
	}
	bool dependency::resolved()
	{
		while(true)
		{
			uint32_t r = resolve_state.load();
			if(r == _unresolved_value())
				return false;
			if(r == _locked_value())
				continue;
			return true;
		}
	}

	uint32_t dependency::get()
	{
		while(true)
		{
			uint32_t r = resolve_state.load();
			if(r == _locked_value())
				continue;
			CDP_ASSERT(r != _unresolved_value());
			return r;
		}
	}

	bool dependency::_isresolved()
	{
		while(true)
		{
			uint32_t r = resolve_state.load();
			if(r == _locked_value())
				continue;
			return r != _unresolved_value();
		}
	}

	void dependency::_attach(coroutine::handle_type h)
	{
		h.promise().next = this->waiting_list;
		this->waiting_list = h;
	}

	coroutine::handle_type dependency::_detach()
	{
		auto result = this->waiting_list;
		this->waiting_list = coroutine::handle_type {};
		return result;
	}
	void dependency::_lock_for_resolve()
	{
		while(true)
		{
			uint32_t pr = resolve_state.exchange(_locked_value(), std::memory_order_acquire);

			if(pr == _unresolved_value())
				break;

			while(true)
			{
				if (pr == _locked_value())
					pr = resolve_state.load(std::memory_order_relaxed);
				else if(pr == _unresolved_value())
					break;
				else
				{
					CDP_ASSERT(false);//dependency already resolved
				}
			}
		}
	}

	bool dependency::_lock_for_await(uint32_t& out)
	{
		while(true)
		{
			uint32_t pr = resolve_state.exchange(_locked_value());

			if(pr == _unresolved_value())
				return true;

			if(pr == _locked_value())
			{
				do
				{
					pr = resolve_state.load();
				}
				while(pr == _locked_value());
			}
			else
			{
				//resolved, put the value back
				out = pr;
				resolve_state.store(pr);
				return false;
			}
		}
	}

	void dependency::_resolve(const uint32_t r)
	{
		uint32_t pr = resolve_state.exchange(r, std::memory_order_release);
		CDP_ASSERT( pr == _locked_value() );
	}
	void dependency::_unlock_unresolved()
	{
		uint32_t pr = resolve_state.exchange(_unresolved_value(), std::memory_order_release);
		CDP_ASSERT( pr == _locked_value() );
	}


}
