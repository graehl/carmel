#ifndef FUNCS_HPP
#define FUNCS_HPP

#include "random.hpp"

#include "memleak.hpp"

#include <iterator>

#include <cmath>
#include <string>
#include <boost/lexical_cast.hpp>

#include <boost/iterator/iterator_adaptor.hpp>
#include <stdexcept>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

#include "debugprint.hpp"

#include <boost/type_traits/alignment_traits.hpp>

#include "myassert.h"
//#include "os.hpp"

#include <functional>
#include <algorithm>
#include <iostream>

template <class size_type,class inputstream>
size_type parse_size(inputstream &i) {    
    size_type size;
    double number;

    if (!(i >> number))
        goto fail;
    char c;
    if (i.get(c)) {
        switch(c) {
        case 't':case 'T':
            number *= (1024.*1024.*1024.*1024.);
            break;
        case 'g':
            number *= (1000*1000*1000);
            break;
        case 'G':
            number *= (1024*1024*1024);
            break;
        case 'm':
            number *= (1000*1000);
            break;
        case 'M':
            number *= (1024*1024);
            break;
        case 'k':
            number *=1000;
            break;
        case 'K':
            number *= 1024;
            break;

        default:
            goto fail;
        }
    }
    if (number - (size_type)number > 1)
        throw std::runtime_error(std::string("Overflow - size too big to fit: ").append(boost::lexical_cast<std::string>(number)));
    return (size_type)number;
fail:    throw std::runtime_error(std::string("Expected nonnegative number followed by optional k,m, or g (2^10,2^20,2^30) suffix."));
}

// requires Val::operator += as well as copy ctor.  TODO: version that takes InPlaceFold functor.
template <class AssocContainer,class Key,class Val>
inline void accumulate(AssocContainer *table,const Key &key,const Val &val) {
    std::pair<typename AssocContainer::iterator,bool> was_inserted=table->insert(typename AssocContainer::value_type(key,val));
    if (!was_inserted.second) {
        typename AssocContainer::value_type::second_type &old_val=was_inserted.first->second;
        old_val += val;
    }
}


template <class AssocContainer,class Key,class Val>
inline void maybe_decrease_min(AssocContainer *table,const Key &key,const Val &val) {
    std::pair<typename AssocContainer::iterator,bool> was_inserted=table->insert(typename AssocContainer::value_type(key,val));
    if (!was_inserted.second) {
        typename AssocContainer::value_type::second_type &old_val=was_inserted.first->second;
//        INFOL(29,"maybe_decrease_min",key << " val=" << val << " oldval=" << old_val);
        if (val < old_val)
            old_val=val;
    } else {
//                INFOL(30,"maybe_decrease_min",key << " val=" << val);
    }
}

template <class AssocContainer,class Key,class Val>
inline void maybe_increase_max(AssocContainer *table,const Key &key,const Val &val) {
    std::pair<typename AssocContainer::iterator,bool> was_inserted=table->insert(typename AssocContainer::value_type(key,val));
    if (!was_inserted.second) {
        typename AssocContainer::value_type::second_type &old_val=was_inserted.first->second;
//        INFOL(29,"maybe_increase_max",key << " val=" << val << " oldval=" << old_val);
        //!FIXME: no idea how to get this << to use ns_Syntax::operator <<
        if (old_val < val)
            old_val=val;
    } else {
//                INFOL(30,"maybe_increase_max",key << " val=" << val);
    }
}

template <class To,class From>
inline void maybe_increase_max(To &to,const From &from) {
    if (to < from)
        to=from;    
}

template <class To,class From>
inline void maybe_decrease_min(To &to,const From &from) {
    if (from < to)
        to=from;    
}

#ifndef ONE_PLUS_EPSILON
# ifndef FLOAT_EPSILON
#  define FLOAT_EPSILON .00001
# endif
static const double EPSILON=FLOAT_EPSILON;
static const double ONE_PLUS_EPSILON=1+EPSILON;

//#define ONE_PLUS_EPSILON (1+EPSILON)
#endif

/*
  The simple solution like abs(f1-f2) <= e does not work for very small or very big values. This floating-point comparison algorithm is based on the more confident solution presented by Knuth in [1]. For a given floating point values u and v and a tolerance e:

| u - v | <= e * |u| and | u - v | <= e * |v|
defines a "very close with tolerance e" relationship between u and v
	(1)

| u - v | <= e * |u| or   | u - v | <= e * |v|
defines a "close enough with tolerance e" relationship between u and v
	(2)

Both relationships are commutative but are not transitive. The relationship defined by inequations (1) is stronger that the relationship defined by inequations (2) (i.e. (1) => (2) ). Because of the multiplication in the right side of inequations, that could cause an unwanted underflow condition, the implementation is using modified version of the inequations (1) and (2) where all underflow, overflow conditions could be guarded safely:

| u - v | / |u| <= e and | u - v | / |v| <= e
| u - v | / |u| <= e or   | u - v | / |v| <= e
	(1`)
(2`)
*/


  //intent: if you want to be conservative about an assert of a<b, test a<(slightly smaller b)
  // if you want a<=b to succeed when a is == b but there were rounding errors so that a+epsilon=b, test a<(slightly larger b)
template <class Float>
inline Float slightly_larger(Float target) {
    return target * ONE_PLUS_EPSILON;
}

template <class Float>
inline Float slightly_smaller(Float target) {
    return target * (1. / ONE_PLUS_EPSILON);
}


// requirement: P::return_type value semantics, default initializes to boolean false (operator !), and P itself copyable (value)
// can then pass finder<P>(P()) to enumerate() just like find(beg,end,P())
template <class P>
struct finder
{
    typedef typename P::return_type return_type;
    typedef finder<P> Self;

    P pred;
    return_type ret;

    finder(const P &pred_): pred(pred_),ret() {}
    finder(const Self &o) : pred(o.pred),ret() {}

    void operator()(const return_type& u)
    {
        if (!ret)
            ret = pred(u);
    }

};

template <class P>
finder<P> make_finder(const P& pred)
{
    return finder<P>(pred);
}


#define INDIRECT_STRUCT_COMMON(type,method) \
    struct indirect_ ## type ## _ ## method                  \
    {                                         \
        typedef type indirect_type;           \
        indirect_ ## type ## _ ## method *pimp;              \
        indirect_ ## type ## _ ## method(type &t) : pimp(&t) {} \
        indirect_ ## type ## _ ## method(const indirect_ ## type ## _ ## method &t) : pimp(t.pimp) {} \

#define INDIRECT_STRUCT(type,method)                         \
    INDIRECT_STRUCT_COMMON(type,method) \
            void operator()() const         \
            {                                   \
                pimp->method();               \
            }                                   \
    };

#define INDIRECT_STRUCT1(type,method,t1,v1)      \
    INDIRECT_STRUCT_COMMON(type,method) \
            void operator()(t1 v1) const         \
            {                                   \
                pimp->method(v1);               \
            }                                   \
    };


#define INDIRECT_STRUCT1(type,method,t1,v1)      \
    INDIRECT_STRUCT_COMMON(type,method) \
            void operator()(t1 v1) const         \
            {                                   \
                pimp->method(v1);               \
            }                                   \
    };



#define INDIRECT_PROTO(type,method) INDIRECT_STRUCT(type,method)    \
        void method()

#define INDIRECT_PROTO1(type,method,t1,v1) INDIRECT_STRUCT1(type,method,t1,v1)    \
        void method(t1 v1)

#define INDIRECT_PROTO2(type,method,t1,v1,t2,v2) INDIRECT_STRUCT2(type,method,t2,v2)   \
        void method(t1 v1,t2 v2)



template <class T>
T *align(T *p)
{
    const unsigned align=boost::alignment_of<T>::value;
    const unsigned align_mask=(align-1);

    Assert2((align_mask & align), ==0); // only works for power-of-2 alignments.
    char *cp=(char *)p;
    cp += align-1;
    cp -= (align_mask & (unsigned)cp);
//    DBP4(sizeof(T),align,(void*)p,(void*)cp);
    return (T*)cp;
}

template <class T>
T *align_down(T *p)
{
    const unsigned align=boost::alignment_of<T>::value;
    const unsigned align_mask=(align-1);
//    DBP3(align,align_mask,align_mask & align);
    //      unsigned & ttop(*(unsigned *)&p); //FIXME: do we need to warn compiler about aliasing here?
//            ttop &= ~align_mask;
    //return p;
    Assert2((align_mask & align), == 0); // only works for power-of-2 alignments.
    unsigned diff=align_mask & (unsigned)p; //= align-(ttop&align_mask)
    return (T*)((char *)p - diff);
}

template <class T>
bool is_aligned(T *p)
{
    //      unsigned & ttop(*(unsigned *)&p); //FIXME: do we need to warn compiler about aliasing here?
    const unsigned align=boost::alignment_of<T>::value;
    const unsigned align_mask=(align-1);
    return !(align_mask & (unsigned)p);
}


#ifdef TEST
#include "test.hpp"
BOOST_AUTO_UNIT_TEST( TEST_FUNC_ALIGN )
{
    unsigned *p;
    p=(unsigned *)0x15;
    BOOST_CHECK_EQUAL(::align(p),(unsigned *)0x18);
    BOOST_CHECK_EQUAL(::align_down(p),(unsigned *)0x14);
    BOOST_CHECK(!is_aligned(p));

    p=(unsigned *)0x16;
    BOOST_CHECK_EQUAL(::align(p),(unsigned *)0x18);
    BOOST_CHECK_EQUAL(::align_down(p),(unsigned *)0x14);
    BOOST_CHECK(!is_aligned(p));

    p=(unsigned *)0x17;
    BOOST_CHECK_EQUAL(::align(p),(unsigned *)0x18);
    BOOST_CHECK_EQUAL(::align_down(p),(unsigned *)0x14);
    BOOST_CHECK(!is_aligned(p));

    p=(unsigned *)0x28;
    BOOST_CHECK_EQUAL(::align(p),p);
    BOOST_CHECK_EQUAL(::align_down(p),p);
    BOOST_CHECK(is_aligned(p));


}

#endif

/*
template <typename T,size_t n>
struct fixed_single_allocator {
//    enum { size = n };
    T space[n];
    T *allocate(size_t _n) {
        Assert(n==_n);
        return space;
    }
    void deallocate(T* tp,size_t _n) {
        Assert(n==_n && tp == space);
    }
};
*/

template <class T>
struct bounded_iterator : public boost::iterator_adaptor<bounded_iterator<T>,T> {
private:
    struct enabler {};
    typedef boost::iterator_adaptor<bounded_iterator<T>,T> Super;
    friend class boost::iterator_core_access;
    void increment() { if (this->base() == end) throw std::out_of_range("bounded iterator incremented past end"); ++this->base_reference(); }
    T end;
public:
    bounded_iterator(const T& begin,const T &end_) : Super(begin), end(end_) {}
/*    template <class O>
    bounded_iterator(bounded_iter<O> const& other, typename boost::enable_if<boost::is_convertible<O*,T*>, enabler>::type = enabler())
    ) : Super(other.base()) {}*/
    bounded_iterator(bounded_iterator<T> const &other) : Super(other),end(other.end) {}
};

namespace nonstd {

// bleh, std::construct also was nonstandard and removed
template <class P,class V>
void construct(P *to,const V& from)
{
  PLACEMENT_NEW(to) P(from);
}

// uninitialized_copy_n was removed from std:: - we don't reuse the name because you might use namespace std; in an old compiler
// plus our version doesn't return the updated iterators
template <class I,class F>
void uninitialized_copy_n(I from, unsigned n, F to)
{
  for ( ; n > 0 ; --n, ++from, ++to)
    //PLACEMENT_NEW(&*to) iterator_traits<F>::value_type(*from);
    construct(&*to,*from);
}
};

struct dummy_nullary_func {
    typedef void result_type;
    void operator()() {}
};

struct tick_writer
{
    typedef void result_type;
    std::ostream *pout;
    std::string tick;
    tick_writer() {}
    tick_writer(const tick_writer &t) : pout(t.pout),tick(t.tick) {}
    tick_writer(std::ostream *o_,const std::string &tick_=".") { init(o_,tick_); }
    void init(std::ostream *o_,const std::string &tick_=".")
    {
        pout=o_;
        tick=tick_;
    }
    void operator()()
    {
        if (pout)
            *pout << tick;
    }
    template <class C>
    void operator()(const C& unused)
    {
        return operator()();
    }
};


struct Periodic {
    unsigned period,left;
    bool enabled;
    Periodic(unsigned period_) {
        set_period(period_);        
    }
    void enable() {
        enabled=true;
    }
    void disable() {
        enabled=false;
    }
    void set_period(unsigned period_=0) {
        enabled=true;
        period=period_;
        trigger_reset();
    }
    void trigger_reset() {
        left=period;
    }
    void trigger_next() {
        left=0;
    }
    bool check() {
        DBP2(period,left);
        if (period && enabled) {
            if (!--left) {
                left=period;
                return true;
            }
        }
        return false;
    }
};

template <class C>
struct periodic_wrapper : public C
{
    typedef C Imp;
    typedef typename Imp::result_type result_type;
    Periodic period;
    periodic_wrapper(unsigned period_=0,const Imp &imp_=Imp()) : period(period_), Imp(imp_) {}
    void set_period(unsigned period_=0)
    {
        period.set_period(period_);
    }
    result_type operator()() {
        if (period.check()) 
            return Imp::operator()();
        return result_type();
    }
    template <class C2>
    result_type operator()(const C2& val) {
        return operator()();
    }
};


// deprecated: use periodic_wrapper<tick_writer> instead
/*
struct progress_ticker {
    typedef void result_type;
    std::ostream *pout;
    unsigned period,left;
    std::string tick;
    progress_ticker() { init(); }
    progress_ticker(std::ostream &o_,unsigned period_=1,const std::string &tick_=".")  { init(&o_); }
    void init(std::ostream *o_=(std::ostream *)NULL,unsigned period_=1,const std::string &tick_=".")  {
        pout=o_;
        period=period_;
        tick=tick_;
        left=period;
    }
    void operator()() {
        DBP4(pout,period,left,tick);
        if (pout) {
            if (!--left) {
                *pout << tick;
                left=period;
            }
        }
    }
};
*/



template <class Forest>
struct self_destruct {
    Forest *me;
    bool safety;
    self_destruct(Forest *me_) : me(me_),safety(false) {}
    void cancel() { safety=true; }
    ~self_destruct() { if (!safety) me->safe_destroy(); }
};

template <class Size=size_t>
struct size_accum {
    Size size;
    Size max_size;

    size_accum() : size(0),max_size(0) {}
    template <class T>
    void operator()(const T& t) {
        Size tsize=t.size();
        size += tsize;
        if (max_size < tsize)
            max_size = tsize;
    }
    Size total() const
    {
        return size;
    }
    Size maximum() const
    {
        return max_size;
    }
    operator Size() const { return total(); }
};

template <class A,class B>
struct both_functors_byref {
    A &a;
    B &b;
    both_functors_byref(A &a_,B &b_) : a(a_),b(b_) {}
    both_functors_byref(const both_functors_byref<A,B> &self) : a(self.a),b(self.b) {}
    template <class C>
    void operator()(C &c)
    {
        a(c);
        b(c);
    }
};

template <class A,class B>
both_functors_byref<A,B> make_both_functors_byref(A &a_,B &b_)
{
    return both_functors_byref<A,B>(a_,b_);
}




/*
      template <class T>
    struct max_accum {
        T m;
        max_accum() : m() {}
        template <class T2>
        void operator()(const T2& t) {
            if (m < t)
                m = t;
        }
        operator T &() { return m; }
    };
*/
template <class T>
struct max_accum {
    T max;
    max_accum() : max() {}
    template <class F>
    void operator()(const F& t) {
        if (max < t)
            max = t;
    }
    operator T &() { return max; }
    operator const T &() const { return max; }
};

template <class T>
struct min_max_accum {
    T max;
    T min;
    bool seen;
    min_max_accum() : seen(false) {}
    template <class F>
    void operator()(const F& t) {
//        DBP3(min,max,t);
        if (seen) {
            if (max < t)
                max = t;
            else if (min > t)
                min = t;
        } else {
            min=max=t;
            seen=true;
        }
//        DBP2(min,max);
    }
};

template <class T>
struct max_in_accum {
    T max;
    max_in_accum() : max() {}
    template <class F>
    void operator()(const F& t) {
        for (typename F::const_iterator i=t.begin(),e=t.end();i!=e;++i)
            if (max < *i)
                max = *i;
    }
    operator T &() { return max; }
    operator const T &() const { return max; }
};


template <class T>
inline std::string concat(const std::string &s,const T& suffix) {
    return s+boost::lexical_cast<std::string>(suffix);
}

template <class S,class T>
inline std::string concat(const S &s,const T& suffix) {
    return boost::lexical_cast<std::string>(s)+boost::lexical_cast<std::string>(suffix);
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
        c=random_pos_fraction();
    }
};



template <class I, class B,class C>
struct indirect_cmp : public C {
    typedef C Comp;
    typedef I Index;
    typedef B Base;
    typedef indirect_cmp<I,B,C> Self;

    B base;

    indirect_cmp(const B &b,const Comp&comp=Comp()) : base(b), Comp(comp) {}
    indirect_cmp(const Self &o): base(o.base), Comp((Comp &)o) {}

    bool operator()(const I &a, const I &b) const {
        return Comp::operator()(base[a], base[b]);
    }
};

template <class I,class B,class C>
void top_n(unsigned n,I begin,I end,const B &base,const C& comp)
{
    indirect_cmp<I,B,C> icmp(base,comp);
    I mid=begin+n;
    if (mid >= end)
        sort(begin,end,icmp);
    else
        partial_sort(begin,mid,end,icmp);
}

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
#include <cctype>
#include "debugprint.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_FUNCS )
{
    using namespace std;
    const int NREP=10000;
    for (int i=1;i<NREP;++i) {
        unsigned ran_lt_i=random_less_than(i);
        BOOST_CHECK(0 <= ran_lt_i && ran_lt_i < i);
        BOOST_CHECK(isalpha(random_alpha()));
        char r_alphanum=random_alphanum();
        BOOST_CHECK(isalpha(r_alphanum) || isdigit(r_alphanum));
    }
}
#endif

#endif
