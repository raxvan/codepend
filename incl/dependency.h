#pragma once

#include "codepend_config.h"

namespace cdp
{

	struct coroutine_pipe;

	struct dependency : public threading::spin_lock
	{
	protected:
		friend struct coroutine_pipe;
		friend struct coroutine_dependency_pool;

		uint32_t resolved_state = 0;

		uint32_t first_task = std::numeric_limits<uint32_t>::max();
	public:
		uint32_t resolve();
		bool     resolved();

	private:
		bool 	 _resolved_locked() const;
		uint32_t _resolve_locked();
	};

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
}
