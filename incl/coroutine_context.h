
#pragma once

#include "codepend_config.h"
#include <iostream>

namespace cdp
{

	struct dependency;

	struct coroutine
	{
		struct coroutine_context;
		using promise_type = coroutine_context;
		using handle_type = std::coroutine_handle<coroutine_context>;

		struct dependency_await
		{
			dependency* waiting_for;

			bool await_ready();

			inline void await_resume()
			{}
			inline constexpr void await_suspend(std::coroutine_handle<coroutine::coroutine_context>)
			{}
		};

		struct coroutine_context
		{
			int32_t refcount = 0;
			dependency* waiting_for = nullptr;

#ifdef CDP_TESTING
			ttf::instance_counter _refcheck;
#endif

			~coroutine_context()
			{
				CDP_ASSERT(refcount == 0 && waiting_for == nullptr);
			}

			/*template <class ... ARGS>
			coroutine_context(controller& _c, ARGS ...)
				:cntrl(&_c)
			{
				std::cout << "constructor" << std::endl;
			}*/

			coroutine get_return_object()
			{
				auto h = handle_type::from_promise(*this);
				return coroutine(std::move(h));
			}
			std::suspend_always initial_suspend()
			{
				return {};
			}
			std::suspend_always final_suspend() noexcept
			{
				return {};
			}
			void unhandled_exception()
			{
				//exception_ = std::current_exception();
			}

			dependency_await await_transform(dependency& d)
			{
				waiting_for = &d;
				return { &d };
			}
			dependency_await await_transform(dependency* dptr)
			{
				waiting_for = dptr;
				return { dptr };
			}
			void return_void()
			{
			}

			/*
			template <std::convertible_to<uint64_t> From>
			std::suspend_always yield_value(From&& from)
			{
				//out = std::forward<From>(from);
				return {};
			}
			*/

			/*
			template <class ... ARGS>
			static void* operator new(size_t size, controller&, ARGS ...)
			{
				return ::operator new(size);
			}

			static void operator delete(void* p)
			{
				::delete(p);
			}
			*/

		};
	public:
		coroutine() = default;
	public:
		coroutine& operator = (const coroutine& other);
		coroutine& operator = (coroutine&& other);
		coroutine(coroutine&& other);
		void swap(coroutine& other);
	public:
		handle_type handle;
	public:
		coroutine(handle_type ht);
		coroutine(const coroutine& other);
		~coroutine();
	};

   



}
