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

	public:
		dependency(const dependency&) = delete;
		dependency(dependency&&) = delete;
		dependency& operator=(const dependency&) = delete;
		dependency& operator=(dependency&&) = delete;

	public:
		bool resolved(uint32_t& out);
		bool resolved();

		void add(coroutine&& co); //add awaiting coroutine on this dependency
		
		coroutine::handle_type resolve(const uint32_t payload = 0);
		coroutine::handle_type detach();//must be unresolved, detaches all coroutines waiting on this

	public:
		uint32_t get();//must be resolved

		void reset();//must be resolved, changes state to unresolved
	protected:
		friend struct coroutine_pipe;
		friend struct coroutine;

		void _lock_for_resolve();
		bool _lock_for_await(uint32_t& out);

		void _unlock_resolve(const uint32_t p);
		void _unlock_unresolved();

		bool _isresolved();

		void _attach(coroutine::handle_type h);
		coroutine::handle_type _detach();
		
	protected:
		std::atomic<uint32_t> 	resolve_state{ std::numeric_limits<uint32_t>::max() };
		coroutine::handle_type 	waiting_list;
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	struct frame
	{
	public:
		~frame() = default;
		frame() = default;

		inline void add(coroutine&& co)
		{
			frame_state.add(std::move(co));
		}
	protected:
		dependency frame_state;
	protected:
		friend struct coroutine_pipe;
		friend struct coroutine;
	};
	

	//--------------------------------------------------------------------------------------------------------------------------------

	template <class T>
	struct result : public dependency
	{
	public:
		T value;

	public:
		result() = default;
		template <std::convertible_to<T> From>
		result(From&& v)
			:value(std::forward<From>(v))
		{
		}
	public:
		template <std::convertible_to<T> From>
		inline coroutine::resolved_dependency_colist_value<T> operator = (From&& v)
		{
			_lock_for_resolve();
			auto rl = _detach();
			value = std::forward<From>(v);
			_unlock_resolve(0);
			return { rl , &value };
		}
	};

	template <>
	struct result <uint32_t> : public dependency
	{
	public:
		inline coroutine::resolved_dependency_colist operator = (const uint32_t& v)
		{
			_lock_for_resolve();
			auto rl = _detach();
			_unlock_resolve(v);
			return { rl };
		}
	};

	//--------------------------------------------------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------------------------------------------------

	inline coroutine::await_on_dependency_value coroutine::coroutine_context::await_transform(result<uint32_t>& d)
	{
		return coroutine::await_on_dependency_value(d, *this);
	}
	inline coroutine::await_on_dependency_value coroutine::coroutine_context::await_transform(result<uint32_t>* dptr)
	{
		CDP_ASSERT(dptr != nullptr);
		return coroutine::await_on_dependency_value(*dptr, *this);
	}

	template <class T>
	inline coroutine::await_on_dependency_result<T> coroutine::coroutine_context::await_transform(result<T>& d)
	{
		return coroutine::await_on_dependency_result<T>(d, *this, &d.value);
	}
	template <class T>
	inline coroutine::await_on_dependency_result<T> coroutine::coroutine_context::await_transform(result<T>* dptr)
	{
		CDP_ASSERT(dptr != nullptr);
		return coroutine::await_on_dependency_result<T>(*dptr, *this, &dptr->value);
	}

	template <class T>
	const T& coroutine::await_on_dependency_result<T>::await_resume()
	{
#ifdef CDP_ENABLE_ASSERT
		if(dependency_ptr != nullptr)
		{
			uint32_t tmp;
			CDP_ASSERT(dependency_ptr->resolved(tmp) == true);
		}
#endif
		CDP_ASSERT(value_ptr != nullptr);
		return *value_ptr;
	}
}
