#pragma once

#include "coroutine_context.h"

namespace cdp
{
	struct coroutine_pipe;

	//--------------------------------------------------------------------------------------------------------------------------------
	struct dependency : public threading::spin_lock
	{
	public:
		void 	 resolve(const uint32_t payload = 0);
		void 	 resolve_in_frame(const uint32_t payload = 0);
		bool     resolved();

		bool 	 owned(const coroutine_pipe* pipe) const;

		coroutine_pipe& pipe();
	public:
		dependency(coroutine_pipe& p)
			:m_owner(&p)
		{}
		~dependency();
		dependency() = default;

		dependency(const dependency&) = delete;
		dependency(dependency&&) = delete;
		dependency& operator = (const dependency&) = delete;
		dependency& operator = (dependency&&) = delete;

	public:
		void set(coroutine_pipe& p);
		void set(coroutine_pipe* p);

	protected:
		friend struct coroutine_pipe;
		friend struct coroutine_dependency_pool;
		
		
	protected:
		bool 	 _isresolved_locked() const;
		
		//uint32_t _resolve(const uint32_t payload);
		//uint32_t _resolve_locked(const uint32_t payload);


	protected:
		coroutine_pipe* 		m_owner = nullptr;
		coroutine::handle_type 	waiting_list;

		uint32_t m_payload = std::numeric_limits<uint32_t>::max();
	};

	//--------------------------------------------------------------------------------------------------------------------------------

	/*
	template <class T>
	struct constref_dependency : public dependency
	{
	protected:
		coroutine_pipe* pipe;
	public:
		T value;

	public:
		constref_dependency(coroutine_pipe* p)
			:pipe(p)
		{
		}
	public:
		void resolve(const T& value);

		inline const T& get()
		{
			CDP_ASSERT(resolved());
			return value;
		}
	};
	*/
}
