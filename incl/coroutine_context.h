
#pragma once

#include "coroutine_signal.h"

namespace cdp
{
	template <class T>
	struct result;

	struct dependency;
	struct frame;

	struct coroutine_pipe;
	struct coroutine;

	struct suspend_context
	{
		using suspend_operator = bool(suspend_context&, coroutine&, coroutine_pipe&);
	};

	struct suspend_data
	{
		suspend_context::suspend_operator* func = nullptr;
		suspend_context*				   context = nullptr;

		bool run(coroutine& co, coroutine_pipe& pipe);
		bool valid() const;
		void reset();

		template <class T>
		void operator << (T* obj)
		{
			func = T::frame_function;
			context = obj;
		};
	};

	struct coroutine
	{
		struct coroutine_context;
		using promise_type = coroutine_context;
		using handle_type = std::coroutine_handle<coroutine_context>;

	public:
		struct coroutine_list : public suspend_context
		{
		public:
			handle_type first = handle_type{};

			inline coroutine_list(handle_type h)
				:first(h)
			{
			}

			coroutine_list() = default;
			coroutine_list(const coroutine_list&) = default;
			coroutine_list& operator =(const coroutine_list&) = default;

		public:
			inline bool empty() const
			{
				return first == handle_type{};
			}
		public://co_await
			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);
			inline bool await_ready()
			{
				return false;
			}
			inline void await_resume()
			{
			}
			inline void await_suspend(coroutine::handle_type h)
			{
				h.promise().frame_function << this;
			}
		};
	public:

		template <class F>
		struct frame_function_await : public suspend_context
		{
			F func;
			inline frame_function_await(F&& _func)
				:func(std::move(_func))
			{}

		public://co_await:
			static bool frame_function(suspend_context& sc, coroutine& co, coroutine_pipe& pipe)
			{
				frame_function_await<F>& d = static_cast<frame_function_await<F>&>(sc);
				return d.func(co, pipe);
			}
			inline bool await_ready()
			{
				return false;
			}
			inline void await_resume()
			{
			}
			inline void await_suspend(coroutine::handle_type h)
			{
				h.promise().frame_function << this;
			}
		};

	public:
		struct coroutine_context
		{
		public:
#ifdef CDP_TESTING
			ttf::instance_counter _refcheck;
#endif
		public:
			int32_t refcount = 1; // 1 for when it's created

			suspend_data frame_function;

			handle_type	 next_parallel;	  // on dependency
			handle_type	 next_sequential;

			cosignal* destroy_signal = nullptr;

		public:
			coroutine_context() = default;
			~coroutine_context();

			handle_type detach_parallel();
			handle_type detach_sequential();

			void add_parallel(coroutine::handle_type h);
			void add_sequential(coroutine::handle_type h);

		public: // other
			coroutine get_return_object()
			{
				return coroutine(handle_type::from_promise(*this));
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
				// std::current_exception();
			}
			void return_void()
			{
			}

			static void* operator new(size_t size);
			static void operator delete(void* p);
			
		};

	public:
		coroutine() = default;

	public:
		coroutine& operator=(const coroutine& other);
		coroutine& operator=(coroutine&& other) noexcept;
		coroutine(coroutine&& other) noexcept;
		void swap(coroutine& other);
		void reset();

		bool valid() const;
	public:
		coroutine(const coroutine& other);
		~coroutine();

		void set_signal(cosignal& csg);
		void set_signal(cosignal* csg);

	public:
		coroutine(const handle_type& ht);
		void		attach(const handle_type& ht);
		handle_type detach();

	public:
		handle_type handle;
	};

	using cohandle = coroutine::handle_type;

	template <class F>
	// bool F(coroutine& co, coroutine_pipe& pipe)
	inline coroutine::frame_function_await<F> frame_function(F&& _func)
	{
		return coroutine::frame_function_await<F>(std::move(_func));
	}

}
