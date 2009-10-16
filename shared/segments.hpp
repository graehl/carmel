#ifndef GRAEHL_SHARED__SEGMENTS_HPP
#define GRAEHL_SHARED__SEGMENTS_HPP

/* segments: sorted list of N+1 endpoints b[i] defining segment id=i as [b[i],b[i+1)], for i in [1,N] */

#include <algorithm>
#include <graehl/shared/dynarray.h>

namespace graehl {

template <class X>
struct segments : public dynamic_array<X>
{
    typedef dynamic_array<X> P;
    typedef typename P::const_iterator const_iterator;

    static const unsigned LEFT=0; // 1 is first valid index (index of upper bound for segment you're in)
    unsigned segid(X x)
    {
        const_iterator w=std::lower_bound(P::begin(),P::end(),x);
        unsigned i=w-P::begin();
        return i;
    }
    unsigned RIGHT()
    {
        return P::size();
    }
    bool increases(X x)
    {
        return P::empty() || x>P::back();
    }
    // deltas and positions must come in increasing order
    void add_at(X x)
    {
        assert(increases(x));
        P::push_back(x);
    }
    void start(X x)
    {
        P::clear();
        P::push_back(x);
    }
    void add_delta(X dx)
    {
        P::push_back(P::empty()?dx:dx+P::back());
    }
    // for integral X only: precompute segid(x) for x=[0,n)
    template <class C>
    void inverse_i_push_back(C &c,X N,X startx=0) const
    {
        X x=startx;
        for (unsigned i=0,N=P::size();i!=N;++i) {
            X xm=(*this)[i];
            for (X M=std::min(N,x);x<xm;++x)
                c.push_back(i+1);
        }
    }
    template <class C>
    void inverse_i(C b,C end,X startx=0) const
    {
        X x=startx;
        for (unsigned i=0,N=P::size();b!=end&&i!=N;++i) {
            X xm=(*this)[i];
            for (;b!=end&&x<xm ;++x)
                *b++=i;
        }
        unsigned R=RIGHT();
        while(b<end) *b++=R;
    }
    segments()
    {
        start(0);
    }
};

}

#endif
