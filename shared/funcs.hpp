#ifndef _FUNCTORS_HPP
#define _FUNCTORS_HPP

#include <cmath>
#include <string>
#include <boost/scoped_ptr.hpp>

inline unsigned random_less_than(unsigned limit) {
// correct against bias (which is worse when limit is almost RAND_MAX)
    const unsigned limit=(RAND_MAX / limit)*limit;
    unsigned r;
    while ((r=std::rand()) >= limit) ;
    return r % limit;
}

// works for ASCII only
inline char random_alpha() {
    return 'A' + rand_less_than(26*2);
}

inline std::string random_alpha_string(unsigned len) {
    boost::scoped_array s(new char[len+1]);
    char *e=s.get()+len;
    *e='\0';
    while(s.get() < e--)
        *e=random_alpha;
    return s.get();
}


inline double random_pos_fraction() // returns uniform random number on (0..1]
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

template <class V>
struct set_value {
    V v;
    set_value(const V& init_value) : v(init_value) {}
    template <class C>
    void operator()(C &c) {
        c=v;
    }
};

template <class V>
set_value<V> value_setter(V &init_value) {
    return set_value<V>(init_value);
}

struct set_random_pos_fraction {
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
#include <cctypes>
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_FUNCTORS )
{
    using namespace std;
    const int NREP=10000;
    for (int i=0;i<NREP;++i) {
        unsigned ran_lt_i=randdom_less_than(i);
        BOOST_CHECK(0 <= ran_lt_i && ran_lt_i < i);
        BOOST_CHECK(isalpha(random_alpha()));
    }
}
#endif

#endif
