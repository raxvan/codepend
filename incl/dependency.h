#pragma once

#include "coroutine_context.h"

namespace cdp
{
	struct coroutine_pipe;

	//--------------------------------------------------------------------------------------------------------------------------------
	struct dependency
	{
	public:
		~dependency();
		dependency() = default;

		dependency(const dependency&) = delete;
		dependency(dependency&&) = delete;
		dependency& operator=(const dependency&) = delete;
		dependency& operator=(dependency&&) = delete;

		bool resolved(uint32_t& out);
		bool resolved();
		void add(coroutine&& co); //add awaiting coroutine on this dependency
		coroutine::handle_type resolve(const uint32_t payload = 0);

		uint32_t get();
	protected:
		friend struct coroutine_pipe;
		friend struct coroutine_dependency_pool;
		friend struct coroutine;

		void _lock_for_resolve();
		void _resolve(const uint32_t p);
		bool _lock_for_await(uint32_t& out);
		void _unlock_unresolved();
		coroutine::handle_type _detach();
		void _attach(coroutine::handle_type h);
		bool _isresolved();
	protected:
		std::atomic<uint32_t> 	resolve_state{ std::numeric_limits<uint32_t>::max() };
		coroutine::handle_type 	waiting_list;
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	template <class T>
	struct result : public dependency
	{
	public:
		T value;

	public:
		inline coroutine::resolved_dependency_yield operator = (const T& v)
		{
			_lock_for_resolve();
			auto rl = _detach();
			value = v;
			_resolve(0);
			return { rl };
		}
		inline coroutine::resolved_dependency_yield operator = (T&& v)
		{
			_lock_for_resolve();
			auto rl = _detach();
			value = std::move(v);
			_resolve(0);
			return { rl };
		}
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	/*template <class T>
	inline coroutine::dependency_await coroutine::coroutine_context::await_transform(result<T>& d)
	{

	}*/

}
