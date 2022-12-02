#pragma once

#include "coroutine_context.h"

namespace cdp
{
	struct coroutine_pipe;

	//--------------------------------------------------------------------------------------------------------------------------------
	struct dependency : public threading::spin_lock
	{
	public:
		~dependency();

		dependency() = default;

		dependency(const dependency&) = delete;
		dependency(dependency&&) = delete;
		dependency& operator=(const dependency&) = delete;
		dependency& operator=(dependency&&) = delete;

		bool resolved();

	protected:
		friend struct coroutine_pipe;
		friend struct coroutine_dependency_pool;
		friend struct coroutine;

		coroutine::handle_type resolve(const uint32_t payload = 0);

	protected:
		bool				   _isresolved_locked() const;
		coroutine::handle_type _resolve_locked(const uint32_t payload);

	protected:
		coroutine::handle_type 	waiting_list;

		uint32_t m_payload = std::numeric_limits<uint32_t>::max();
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
			coroutine::handle_type rl;
			{
				std::lock_guard<threading::spin_lock> _(*this);
				value = v;
				rl = _resolve_locked(0);
			}
			return { rl };
		}
		inline coroutine::resolved_dependency_yield operator = (T&& v)
		{
			coroutine::handle_type rl;
			{
				std::lock_guard<threading::spin_lock> _(*this);
				value = std::move(v);
				rl = _resolve_locked(0);
			}
			return { rl };
		}
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	/*template <class T>
	inline coroutine::dependency_await coroutine::coroutine_context::await_transform(result<T>& d)
	{

	}*/

}
