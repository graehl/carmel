#ifndef GRAEHL__SHARED__PERCENT_HPP
#define GRAEHL__SHARED__PERCENT_HPP

#include <graehl/shared/stream_util.hpp>

namespace graehl {

template <int width=5>
struct percent
{
    double frac;
    percent(double f) : frac(f) {}
    percent(double num,double den) : frac(num/den) {}
    double get_percent() const 
    {
        return frac*100;
    }
    template <class O> void print(O &o) const
    {
        print_max_width_small(o,get_percent(),width-1);
        o << '%';
    }
    typedef percent<width> self_type;
    TO_OSTREAM_PRINT
};

    
}


#endif
