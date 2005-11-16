#ifndef ASSERTLVL_HPP
#define ASSERTLVL_HPP

#ifndef ASSERT_LEVEL
# define ASSERT_LEVEL 9999
#endif

#include <cassert>

#define IF_ASSERT(level) if(ASSERT_LEVEL>=level)
#define UNLESS_ASSERT(level) if(ASSERT_LEVEL<level)
#define assertlvl(level,assertion) IF_ASSERT(level) {assert(assertion);}

#endif
