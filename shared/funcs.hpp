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


// useful for sorting; could parameterize on predicate instead of just <=lt, >=gt
template <class I, class B>
struct indirect_lt {
    typedef I Index;
    typedef B Base;
    B base;
    indirect_lt(const B &b) : base(b) {}
    indirect_lt(const indirect_lt<I,B> &o): base(o.base) {}

    bool operator()(const I &a, const I &b) const {
        return base[a] < base[b];
    }
};


template <class I, class B>
struct indirect_gt {
    typedef I Index;
    typedef B Base;
    B base;
    indirect_gt(const B &b) : base(b) {}
    indirect_gt(const indirect_gt<I,B> &o): base(o.base) {}
    bool operator()(const I &a, const I &b) const {
        return base[a] > base[b];
    }
};

  template <class ForwardIterator>
  bool is_sorted(ForwardIterator begin, ForwardIterator end)
  {
    if (begin == end) return true;

    ForwardIterator next = begin ;
    ++next ;
    for ( ; next != end; ++begin , ++next) {
      if (*next < *begin) return false;
    }

    return true;
  }

  template <class ForwardIterator, class StrictWeakOrdering>
  bool is_sorted(ForwardIterator begin, ForwardIterator end,
                 StrictWeakOrdering comp)
  {
    if (begin == end) return true;

    ForwardIterator next = begin ;
    ++next ;
    for ( ; next != end ; ++begin, ++next) {
      if ( comp(*next, *begin) ) return false;
    }

    return true;
  }

  template <class ForwardIterator, class ValueType >
  void iota(ForwardIterator begin, ForwardIterator end, ValueType value)
  {
    while ( begin != end ) {
      *begin = value ;
      ++begin ;
      ++value ;
    }
  }

#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_FUNCTORS )
{
}
#endif

#endif
