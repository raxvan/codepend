
#pragma once

#include "dependency.h"

namespace cdp
{

	struct coroutine
	{
		struct coroutine_context;
		using promise_type = coroutine_context;
		using handle_type = std::coroutine_handle<coroutine_context>;

		struct dependency_await
		{
			dependency* awaiting_dependency;

			bool await_ready();
			void await_resume(); //the result of await_resume is the result of `co_await EXPR`

			inline constexpr void await_suspend(std::coroutine_handle<coroutine::coroutine_context>)
			{}
		};

		struct coroutine_context
		{
		public:
			int32_t refcount = 0;
			dependency* waiting_for = nullptr;

			threading::barrier* on_destroy = nullptr;
#ifdef CDP_TESTING
			ttf::instance_counter _refcheck;
#endif

		public:
			//await modifiers: https://en.cppreference.com/w/cpp/language/coroutines#co_await
			dependency_await await_transform(dependency& d);
			dependency_await await_transform(dependency* dptr);

		public:
			coroutine_context() = default;
			~coroutine_context();

		public: //other 

			/*template <class ... ARGS>
			coroutine_context(controller& _c, ARGS ...)
				:cntrl(&_c)
			{
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
				//TODO
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

	public:
		coroutine& operator >> (threading::barrier& b);
	};

   



}
