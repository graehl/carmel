#ifndef GRAEHL__SHARED__ABS_INT_HPP
#define GRAEHL__SHARED__ABS_INT_HPP

#include <boost/cstdint.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/remove_cv.hpp>

namespace graehl {

template <class I>
inline typename boost::enable_if< typename boost::is_integral<I>
                                , typename boost::remove_cv<I>::type
                                >::type 
bit_rotate_right(I x)
{
    typedef typename boost::remove_cv<I>::type IT;
    return x<0?-x:x;
}

}


#endif
