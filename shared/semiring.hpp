#ifndef SEMIRING_HPP
#define SEMIRING_HPP


/*template <class C>
struct semiring_traits {
    typedef C value_type;
    static inline value_type exponential(double exponent) {
        return exponential<C>(exponent);
    }
    static inline value_type exponential(float exponent) {
        return exponential<C>(exponent);
    }
};
*/

#include "weight.h"


#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_SEMIRING )
{
}
#endif

#endif
