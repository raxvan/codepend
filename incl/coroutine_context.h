
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
	};

	struct coroutine
	{
		struct coroutine_context;
		using promise_type = coroutine_context;
		using handle_type = std::coroutine_handle<coroutine_context>;

	public:
		struct resolved_dependency_colist
		{
			handle_type resolve_list;
		};

		template <class T>
		struct resolved_dependency_colist_value
		{
			handle_type resolve_list;
			const T*	value_ptr;
		};

		struct await_on_dependency_impl : public suspend_context
		{
			dependency* dependency_ptr;

			bool await_ready();

			inline constexpr void await_suspend(handle_type)
			{
			}
		};

		struct await_on_resolve_impl : public suspend_context
		{
			handle_type resolve_list;

			bool await_ready();

			inline constexpr void await_suspend(handle_type)
			{
			}
		};

		struct await_on_dependency_base : public await_on_dependency_impl
		{
			await_on_dependency_base(dependency& d, coroutine_context& coctx);
			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);
		};
		struct await_on_dependency : public await_on_dependency_base
		{
			inline await_on_dependency(dependency& d, coroutine_context& coctx)
				: await_on_dependency_base(d, coctx)
			{
			}

			void await_resume(); // the result of await_resume is the result of `co_await EXPR`
		};

		struct await_on_frame : public suspend_context
		{
			frame* frame_ptr;

			inline constexpr bool await_ready()
			{
				return false;
			}
			inline constexpr void await_suspend(handle_type)
			{
			}
			inline constexpr void await_resume()
			{
			}

			await_on_frame(frame& f, coroutine_context& coctx);
			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);
		};

		struct await_on_dependency_value : public await_on_dependency_impl
		{
			uint32_t result;

			await_on_dependency_value(dependency& d, coroutine_context& coctx);

			uint32_t await_resume(); // the result of await_resume is the result of `co_await EXPR`

			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);
		};

		template <class T>
		struct await_on_dependency_result : public await_on_dependency_base
		{
			T* value_ptr;

			inline T& await_resume();

			inline await_on_dependency_result(dependency& d, coroutine_context& coctx, T* _value_ptr)
				: await_on_dependency_base(d, coctx)
				, value_ptr(_value_ptr)
			{
			}
		};

		struct await_on_resolve : public await_on_resolve_impl
		{
			await_on_resolve(handle_type colist, coroutine_context& coctx);

			constexpr void await_resume()
			{
			}
			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);
		};

		template <class T>
		struct await_on_resolve_value : public await_on_resolve
		{
			const T* value_ptr;
			inline await_on_resolve_value(handle_type colist, coroutine_context& coctx, const T* _value_ptr)
				: await_on_resolve(colist, coctx)
				, value_ptr(_value_ptr)
			{
			}
			inline const T& await_resume()
			{
				CDP_ASSERT(value_ptr != nullptr);
				return *value_ptr;
			}
		};

		template <class F>
		// func: coroutine F()
		struct await_on_frame_function : public suspend_context
		{
			F func;
			inline await_on_frame_function(F&& _func, coroutine_context& coctx);

			inline constexpr bool await_ready()
			{
				return false;
			}
			inline constexpr void await_suspend(handle_type)
			{
			}
			inline constexpr void await_resume()
			{
			}
			// static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);
		};

		template <class F>
		// func: bool F(coroutine& co, coroutine_pipe& pipe)
		struct yield_frame_function
		{
			F func;
			using class_t = yield_frame_function<F>;

			inline yield_frame_function(F&& _func)
				: func(std::move(_func))
			{
			}
			inline yield_frame_function(class_t&& other)
				: func(std::move(other.func))
			{
			}

			yield_frame_function(const class_t&) = delete;
			class_t& operator=(class_t&& other) = delete;
			class_t& operator=(const class_t&) = delete;
		};

		template <class F>
		// bool F(coroutine& co, coroutine_pipe& pipe)
		static yield_frame_function<F> frame_function(F&& _func)
		{
			return yield_frame_function<F>(std::move(_func));
		}

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
			handle_type	 next_sequential; // on dependency

			cosignal* destroy_signal = nullptr;

		public:
			// await modifiers: https://en.cppreference.com/w/cpp/language/coroutines#co_await
			inline await_on_dependency await_transform(dependency& d)
			{
				return await_on_dependency(d, *this);
			}
			inline await_on_dependency await_transform(dependency* dptr)
			{
				CDP_ASSERT(dptr != nullptr);
				return await_on_dependency(*dptr, *this);
			}
			inline await_on_frame await_transform(frame& f)
			{
				return await_on_frame(f, *this);
			}
			inline await_on_frame await_transform(frame* fptr)
			{
				CDP_ASSERT(fptr != nullptr);
				return await_on_frame(*fptr, *this);
			}
			inline await_on_dependency_value await_transform(result<uint32_t>& d);
			inline await_on_dependency_value await_transform(result<uint32_t>* dptr);

			template <class T>
			inline await_on_dependency_result<T> await_transform(result<T>& dptr);
			template <class T>
			inline await_on_dependency_result<T> await_transform(result<T>* dptr);

			await_on_resolve yield_value(dependency& d);
			await_on_resolve yield_value(dependency* dptr);
			await_on_resolve yield_value(resolved_dependency_colist dy);

			template <class T>
			inline await_on_resolve_value<T> yield_value(resolved_dependency_colist_value<T> dy)
			{
				return await_on_resolve_value<T>(dy.resolve_list, *this, dy.value_ptr);
			}

			template <class F>
			inline await_on_frame_function<F> yield_value(yield_frame_function<F> yv)
			{
				return await_on_frame_function<F>(std::move(yv.func), *this);
			}

		public:
			coroutine_context() = default;
			~coroutine_context();

			handle_type detach_parallel();
			handle_type detach_sequential();

			void add_parallel(coroutine::handle_type h);
			void add_sequential(coroutine::handle_type h);

		public: // other
			/*template <class ... ARGS>
			coroutine_context(controller& _c, ARGS ...)
				:cntrl(&_c)
			{
			}*/

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

			/*
			template <std::convertible_to<uint64_t> From>
			std::suspend_always yield_value(From&& from)
			{
				//out = std::forward<From>(from);
				return {};
			}
			*/

			static void* operator new(size_t size);
			static void operator delete(void* p);

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
		coroutine& operator=(const coroutine& other);
		coroutine& operator=(coroutine&& other);
		coroutine(coroutine&& other);
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

}
