#ifndef _FUNCTORS_HPP
#define _FUNCTORS_HPP

#include <cmath>

inline double rand_pos_fraction() // returns uniform random number on (0..1]
{
    return ((double)std::rand()+1.) /
        ((double)RAND_MAX+1.);
}

struct set_one {
    template <class C>
    void operator()(C &c) {
        c=1;
    }
};

struct set_zero {
    template <class C>
    void operator()(C &c) {
        c=0;
    }
};

struct set_rand_pos_fraction {
    template <class C>
    void operator()(C &c) {
        c=rand_pos_fraction();
    }
};



#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_FUNCTORS )
{
}
#endif

#endif
