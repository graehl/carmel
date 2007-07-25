#ifndef GRAEHL__SHARED__MEAN_FIELD_SCALE_HPP
#define GRAEHL__SHARED__MEAN_FIELD_SCALE_HPP

#include <digamma.hpp>
#include <weight.h>

namespace graehl {

struct mean_field_scale
{
    bool linear; // if linear, then don't use alpha.  otherwise convert to exp(digamma(alpha+x))
    double alpha;
    mean_field_scale() : linear(true),alpha(0) {}
    
    // returns exp(digamma(x))
    template <class Real>
    logweight<Real> operator()(logweight<Real> const& x) const 
    {
        if (linear)
            return x;
        typedef logweight<Real> W;
        W xa=x+alpha;
        double floor=.0002;
        if (xa < floor) // until we can compute digamma in logspace, this will be the answer.  and, can't ask digamma(0), because it's negative inf.  but exp(-inf)=0
            return W(digamma(floor),false)*(xa/floor);
        return W(digamma(xa.getReal()),false);
    }
};

}


#endif
