
#pragma once

#include "coroutine_signal.h"

namespace cdp
{
	struct dependency;
	struct coroutine_pipe;
	struct coroutine;

	struct suspend_context
	{
		using suspend_operator = bool(suspend_context&, coroutine&, coroutine_pipe&, const bool recursive);
	};

	struct suspend_data
	{
		suspend_context::suspend_operator* func = nullptr;
		suspend_context* context = nullptr;

		bool run(coroutine& co, coroutine_pipe& pipe, const bool recursive);
		bool valid() const;
		void reset();
	};

	struct coroutine
	{
		struct coroutine_context;
		using promise_type = coroutine_context;
		using handle_type = std::coroutine_handle<coroutine_context>;

	public:

		struct dependency_await : public suspend_context
		{
			dependency* awaiting_dependency;

			dependency_await(dependency& d, coroutine_context& coctx);

			bool await_ready();
			void await_resume(); //the result of await_resume is the result of `co_await EXPR`

			inline constexpr void await_suspend(handle_type)
			{}

			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&, const bool recursive);
		};

		struct dependency_resolve : public suspend_context
		{
			handle_type resolve_list;

			dependency_resolve(dependency& d, coroutine_context& coctx, const uint32_t payload = 0);

			bool await_ready();
			
			constexpr void await_resume()
			{}
			constexpr void await_suspend(handle_type)
			{}

			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&, const bool recursive);
		};

	public:

		struct coroutine_context
		{
		public:
#ifdef CDP_TESTING
			ttf::instance_counter _refcheck;
#endif
		public:
			int32_t refcount = 1; //1 for when it's created

			suspend_data 	frame_function;
			handle_type 	next;

			cosignal*   	destroy_signal = nullptr;

		public:
			//await modifiers: https://en.cppreference.com/w/cpp/language/coroutines#co_await
			inline dependency_await await_transform(dependency& d)
			{
				return dependency_await(d, *this);
			}
			inline dependency_await await_transform(dependency* dptr)
			{
				CDP_ASSERT(dptr != nullptr);
				return dependency_await(*dptr, *this);	
			}
			inline dependency_resolve yield_value(dependency& d)
			{
				return dependency_resolve(d, *this);
			}
			inline dependency_resolve yield_value(dependency* dptr)
			{
				CDP_ASSERT(dptr != nullptr);
				return dependency_resolve(*dptr, *this);	
			}

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
		void reset();
	public:
		handle_type handle;
	public:
		coroutine(const coroutine& other);
		~coroutine();
	protected:
		friend struct coroutine_pipe;
		friend struct dependency;

		coroutine(const handle_type& ht);
		void attach(const handle_type& ht);
		handle_type detach();
	};

	using cohandle = coroutine::handle_type;

}
