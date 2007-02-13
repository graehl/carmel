#ifndef GRAEHL__SHARED__ABS_INT_HPP
#define GRAEHL__SHARED__ABS_INT_HPP

#include <boost/cstdint.hpp>

namespace graehl {

#define GRAEHL__ABS_INT(t) inline t abs(t x) { return x<0 ? -x : x; }
    
GRAEHL__ABS_INT(boost::int8_t)
GRAEHL__ABS_INT(boost::int16_t)
GRAEHL__ABS_INT(boost::int32_t)
GRAEHL__ABS_INT(boost::int64_t)

}


#endif
