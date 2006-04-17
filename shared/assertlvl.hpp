#ifndef GRAEHL__SHARED__ASSERTLVL_HPP
#define GRAEHL__SHARED__ASSERTLVL_HPP

#ifndef ASSERT_LEVEL
# define ASSERT_LEVEL 9999
#endif

#define IF_ASSERT(level) if(ASSERT_LEVEL>=level)
#define UNLESS_ASSERT(level) if(ASSERT_LEVEL<level)
#ifndef assertlvl
#include <cassert>
#define assertlvl(level,assertion) do { IF_ASSERT(level) {assert(assertion);} } while(0)
#endif
#endif
