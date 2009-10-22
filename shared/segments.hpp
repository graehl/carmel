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
    unsigned segid0based(X const& x) const
    {
        return segid(x)-1;
    }

    unsigned segid(X const& x) const
    {
//        if (P::front()==x) return 1;
        const_iterator w=std::upper_bound(P::begin(),P::end(),x); // returns one past last equal or lesser item.
        unsigned i=w-P::begin();
        return i;
    }
    unsigned RIGHT() const
    {
        return P::size();
    }
    bool increases(X const& x) const
    {
        return P::empty() || x>P::back();
    }
    // deltas and positions must come in increasing order
    void add_at(X const& x)
    {
        assert(increases(x));
        P::push_back(x);
    }
    void set_start(X const& x)
    {
        P::clear();
        P::push_back(x);
    }
    void add_delta(X const& dx)
    {
        P::push_back(P::empty()?dx:dx+P::back());
    }
    // for integral X only: precompute segid(x) for x=[0,n)
    template <class C>
    void inverse_i_push_back(C &c,X const& N,X const& startx=0) const
    {
        X x=startx;
        for (unsigned i=0,N=P::size();i!=N;++i) {
            X xm=(*this)[i];
            for (X M=std::min(N,x);x<xm;++x)
                c.push_back(i+1);
        }
    }
    template <class C>
    void inverse_i(C b,C end,X const& startx=0) const
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
        set_start(0);
    }
};

}

#endif
