
#include <codepend_config.h>

#ifdef CDP_ASSERT_ENABLED
#	include <iostream>
#	include <cassert>
#endif

namespace cdp
{

#ifdef CDP_ASSERT_ENABLED
	void codepend_assert_failed(const char* file, const int line, const char* cond)
	{
		std::cerr << "CDP_ASSERT failed in " << file << "(" << line << ")> " << cond << std::endl;
		assert(false);
	}
#endif

}