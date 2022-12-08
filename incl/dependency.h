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

		void add(coroutine&& co); // add awaiting coroutine on this dependency

		coroutine::coroutine_list resolve(const uint32_t payload = 0);
		coroutine::coroutine_list detach(); // must be unresolved, detaches all coroutines waiting on this

	public:
		uint32_t get(); // must be resolved

		void reset(); // must be resolved, changes state to unresolved

	protected:
		friend struct coroutine_pipe;
		friend struct coroutine;

		void _lock_for_resolve();
		bool _lock_for_await(uint32_t& out);

		void _unlock_resolve(const uint32_t p);
		void _unlock_unresolved();

		bool _isresolved();

		void				   _attach(coroutine::handle_type h);
		coroutine::handle_type _detach();

	protected:
		std::atomic<uint32_t>  resolve_state { std::numeric_limits<uint32_t>::max() };
		coroutine::handle_type waiting_list = coroutine::handle_type{};
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

		inline coroutine::coroutine_list detach_waiting_list()
		{
			return frame_state.detach();
		}
	protected:
		dependency frame_state;
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
			: value(std::forward<From>(v))
		{
		}

		template <std::convertible_to<T> From>
		inline coroutine::coroutine_list resolve(From&& v)
		{
			_lock_for_resolve();
			auto rl = _detach();
			value = std::forward<From>(v);
			_unlock_resolve(0);
			return rl;
		}

	};

	template <>
	struct result<uint32_t> : public dependency
	{
	public:
		inline coroutine::coroutine_list resolve(const uint32_t& v)
		{
			_lock_for_resolve();
			auto rl = _detach();
			_unlock_resolve(v);
			return rl;
		}
	};

	//--------------------------------------------------------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------------------------------------------------------

	template <class T>
	inline coroutine::await_on_result<T> coroutine::coroutine_context::await_transform(result<T>& d)
	{
		return coroutine::await_on_result<T>(d, *this, d.value);
	}
	template <class T>
	inline coroutine::await_on_result<T> coroutine::coroutine_context::await_transform(result<T>* dptr)
	{
		CDP_ASSERT(dptr != nullptr);
		return coroutine::await_on_result<T>(*dptr, *this, dptr->value);
	}

	template <class T>
	inline const T& coroutine::await_on_result<T>::await_resume() const
	{
		CDP_ASSERT(dependency_ptr == nullptr || dependency_ptr->_isresolved() == true);
		return valueref;
	}
}
