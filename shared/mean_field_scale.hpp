#ifndef GRAEHL__SHARED__MEAN_FIELD_SCALE_HPP
#define GRAEHL__SHARED__MEAN_FIELD_SCALE_HPP

#include <graehl/shared/digamma.hpp>
#include <graehl/shared/weight.h>

namespace graehl {

struct mean_field_scale
{
    bool linear; // if linear, then don't use alpha.  otherwise convert to exp(digamma(alpha+x))
    double alpha;
    mean_field_scale() { set_default(); }
    void set_default()
    {
        linear=true;
        alpha=0;
    }
    void set_alpha(double a)
    {
        linear=false;
        alpha=a;
    }

    // returns exp(digamma(x))
    template <class Real>
    logweight<Real> operator()(logweight<Real> const& x) const
    {
        if (linear)
            return x;
        typedef logweight<Real> W;
        double xa=x.getReal()+alpha;
        const double floor=.0002;
        static const W dig_floor(digamma(floor),false);
        if (xa < floor) // until we can compute digamma in logspace, this will be the answer.  and, can't ask digamma(0), because it's negative inf.  but exp(-inf)=0
            return dig_floor*(xa/floor);
        // this is a mistake: denominator of sum of n things is supposed to get (alpha*n + sum), not (alpha+sum).  but it seems to work better (sometimes)
        return W(digamma(xa),false);
    }
};

}


#endif
