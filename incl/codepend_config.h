
#pragma once

#define CDP_ENABLE_ASSERT

//--------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------
#if defined(CDP_TESTING)

#	include <ttf.h>
#	define CDP_ASSERT TTF_ASSERT

#elif defined(CDP_ENABLE_ASSERT)
//--------------------------------------------------------------------------------------------------------------------------------
#	if defined(CDP_WITH_DEV_PLATFORM)

#		include <devtiny.h>
#		define CDP_ASSERT DEV_ASSERT

#	else

//--------------------------------------------------------------------------------------------------------------------------------

namespace cpd
{
	extern "C++" void codepend_assert_failed(const char* file, const int line, const char* cond);
}

#		define CDP_ASSERT(_COND)                                  \
			do                                                       \
			{                                                        \
				if (!(_COND))                                        \
					codepend_assert_failed(__FILE__, __LINE__, #_COND); \
			} while (false)
#		define CDP_ASSERT_FALSE(CSTR_MSG)                       \
			do                                                     \
			{                                                      \
				codepend_assert_failed(__FILE__, __LINE__, CSTR_MSG); \
			} while (false)
#	endif
//--------------------------------------------------------------------------------------------------------------------------------
#endif
//--------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------------

#ifndef CDP_ASSERT

#	define CDP_ASSERT(...) \
		do                    \
		{                     \
		} while (false)
#endif

#ifndef CDP_ASSERT_FALSE
#	define CDP_ASSERT_FALSE(...) \
		do                          \
		{                           \
			CDP_ASSERT(false);    \
		} while (false)

#endif

//--------------------------------------------------------------------------------------------------------------------------------

#include <limits>
#include <coroutine>

namespace cdp
{
	using dependency_handle_t = uint32_t;

	constexpr uint32_t invalid_dependency_handle()
	{
		return std::numeric_limits<uint32_t>::max();
	}
}