
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
		struct coroutine_list
		{
			handle_type first = handle_type{};

			inline coroutine_list(handle_type h)
				:first(h)
			{
			}

			coroutine_list() = default;
			coroutine_list(const coroutine_list&) = default;
			coroutine_list& operator =(const coroutine_list&) = default;
		};
	public:
		struct await_suspend_with_colist : public suspend_context
		{
			coroutine_list list;
			await_suspend_with_colist(handle_type colist, coroutine_context& coctx);

			bool await_ready();

			constexpr void await_resume()
			{
			}
			constexpr void await_suspend(handle_type)
			{
			}
			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);
		};
	public:
		struct await_on_dependency : public suspend_context
		{
			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);

			dependency* dependency_ptr;

			await_on_dependency(dependency& d, coroutine_context& coctx);

			bool await_ready();
			void await_resume();

			inline constexpr void await_suspend(handle_type)
			{
			}
		};
		struct await_on_dependency_value : public suspend_context
		{
			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);
			dependency* dependency_ptr;
			uint32_t result;

			await_on_dependency_value(dependency& d, coroutine_context& coctx);

			bool await_ready();
			uint32_t await_resume();

			inline constexpr void await_suspend(handle_type)
			{
			}
		};

		struct await_on_frame : public suspend_context
		{
			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);
			frame* frame_ptr;

			await_on_frame(frame& f, coroutine_context& coctx);

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
		};


		/*struct await_on_result_impl 
		{
			await_on_result_impl(dependency& d, coroutine_context& coctx);
			dependency* dependency_ptr;
		}*/
		template <class T>
		struct await_on_result : private await_on_dependency
		{
			const T& valueref;

			inline bool await_ready()
			{
				return await_on_dependency::await_ready();
			}
			constexpr void await_suspend(handle_type)
			{
			}
			inline const T& await_resume() const;
			
			inline await_on_result(dependency& d, coroutine_context& coctx, const T& _value)
				:await_on_dependency(d, coctx)
				,valueref(_value)
			{
			}
		};

		
		template <class F>
		struct await_suspend_frame_function : public suspend_context
		{
			F func;
			inline await_suspend_frame_function(F&& _func, coroutine_context& coctx);

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

			static bool frame_function(suspend_context&, coroutine&, coroutine_pipe&);
		};

		template <class F>
		struct frame_function_transport
		{
			F func;
			inline frame_function_transport(F&& _func)
				:func(std::move(_func))
			{}
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
			await_suspend_with_colist await_transform(coroutine_list colist);
			await_on_dependency await_transform(dependency& d);
			await_on_dependency await_transform(dependency* dptr);
			await_on_frame await_transform(frame& f);
			await_on_frame await_transform(frame* fptr);
			
		public:
			await_on_dependency_value await_transform(result<uint32_t>& d);
			await_on_dependency_value await_transform(result<uint32_t>* dptr);

			template <class T>
			inline await_on_result<T> await_transform(result<T>& dptr);
			template <class T>
			inline await_on_result<T> await_transform(result<T>* dptr);
			

			template <class F>
			inline await_suspend_frame_function<F> await_transform(frame_function_transport<F> ft)
			{
				return await_suspend_frame_function<F>(std::move(ft.func), *this);	
			}

		public:
			

			//template <class T>
			//inline void await_transform(std::pair<coroutine_list,T> colist);

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

	template <class F>
	// bool F(coroutine& co, coroutine_pipe& pipe)
	inline coroutine::frame_function_transport<F> frame_function(F&& _func)
	{
		return coroutine::frame_function_transport<F>(std::move(_func));
	}

}
