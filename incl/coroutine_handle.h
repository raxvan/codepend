
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


		struct coroutine_context
		{
			int32_t refcount = 0;
			dependency* waiting_for = nullptr;

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


			/*
			template <std::convertible_to<uint64_t> From>
			std::suspend_always yield_value(From&& from)
			{
				//out = std::forward<From>(from);
				return {};
			}
			*/
			void return_void()
			{
			}

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
		
		coroutine& operator = (const coroutine& other)
		{
			coroutine tmp(other);
			swap(tmp);
			return (*this);
		}
		coroutine& operator = (coroutine&& other)
		{
			coroutine tmp;
			swap(tmp);
			swap(other);
			return (*this);
		}
		coroutine(coroutine&& other)
		{
			swap(other);
		}

		void swap(coroutine& other)
		{
			std::swap(handle, other.handle);
		}
	public:
		handle_type handle;
		coroutine(handle_type ht)
			:handle(std::move(ht))
		{
			handle.promise().refcount++;
		}
		coroutine(const coroutine& other)
			:handle(other.handle)
		{
			if (handle)
			{
				handle.promise().refcount++;
			}
		}
		~coroutine()
		{
			if(handle)
			{
				if((--handle.promise().refcount) == 0)
				{
					handle.destroy();
				}
			}
		}

	};

   



}