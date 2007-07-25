#ifndef GRAEHL__SHARED__MEAN_FIELD_NORMALIZE_HPP
#define GRAEHL__SHARED__MEAN_FIELD_NORMALIZE_HPP

#include <digamma.hpp>
#include <weight.h>

namespace graehl {

struct mean_field_scale
{
    bool linear; // if linear, then don't use alpha.  otherwise convert to exp(digamma(alpha+x))
    double alpha;

    // returns exp(digamma(x))
    template <class Real>
    logweight<Real> operator()(logweight<Real> const& x) const 
    {
        if (linear)
            return x;
        double r=x.getReal();
        if (x < .0001) // until we can compute digamma in logspace, this will be the answer.  and, can't ask digamma(0), because it's negative inf.  but exp(-inf)=0
            return 0;
        logweight<Real> ret;
        ret.setLn(digamma(alpha+r));
    }
};

}


#endif
