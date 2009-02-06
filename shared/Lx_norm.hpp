#ifndef GRAEHL_SHARED__LX_NORM_HPP
#define GRAEHL_SHARED__LX_NORM_HPP

#include <graehl/shared/reduce.hpp>

namespace graehl {

struct sum_powx
{
    double x;

    // most common is L2-norm (Euclid. distance)
    sum_powx(double x=2) : x(x) {}
    
    template <class W>
    W operator()(W total,W component) 
    {
        return total+pow(component,x);
    }

    //boost::result_of
    template <class W> struct result {};
    template <class W> struct result<Lx_norm(W,W> { typedef W type; };
};

template <class R>
typename range_value<R>::type
lx_norm(R const& range,double x=2) 
{
    return pow(reduce(range,sum_powx(x),0),1./x);
}

    

}


#endif
