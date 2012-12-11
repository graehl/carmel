// misc. helpful template functions (TODO: categorize them)
#ifndef GRAEHL_SHARED__FUNCS_HPP
#define GRAEHL_SHARED__FUNCS_HPP

#include <iterator>

#include <cmath>
#include <string>
#include <boost/lexical_cast.hpp>

#include <boost/iterator/iterator_adaptor.hpp>
#include <stdexcept>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

#include <graehl/shared/debugprint.hpp>

#include <graehl/shared/myassert.h>
//#include "os.hpp"

#include <functional>
#include <algorithm>
#include <iostream>
#include <cstddef>

#include <vector>

#include <graehl/shared/size_mega.hpp> //FIXME: remove (fixing users)

namespace graehl {

template <class V>
struct difference_f
{
    V operator()(const V&l,const V&r) const
    {
        return l-r;
    }
};

template <class V,class S,class F>
inline S transform2_array_coerce(const S&l,const S&r,F f)
{
    const unsigned N=sizeof(S)/sizeof(V);
    S ret;
    const V *pl=(V*)&l;
    const V *pr=(V*)&r;
    V *pret=(V*)&ret;
    for (unsigned i=0;i<N;++i)
        pret[i]=f(pl[i],pr[i]);
    return ret;
}

template <class T1, class T2>
T1 at_least(const T1 &val,const T2 &ensure_min)
{
    return (val < ensure_min) ? ensure_min : val;
}

template <class T1, class T2>
T1 at_most(const T1 &val,const T2 &ensure_max)
{
    return (val > ensure_max) ? ensure_max : val;
}


template <class Set>
struct not_in_set
{
    const Set *set;
    not_in_set(const Set *set_) : set(set_) {}
    not_in_set(const not_in_set<Set> &o) : set(o.set) {}
    typedef bool result_type;
    template <class V>
    bool operator()(const V& v) const
    {
        return set->find(v)==set->end();
    }
};

template <class Set>
not_in_set<Set> make_not_in_set(Set *s)
{
    return not_in_set<Set>(s);
}

template <class Container,class Predicate>
void remove_if(Container &cont,Predicate pred)
{
    cont.erase(std::remove_if(cont.begin(),cont.end(),pred),cont.end());
}

template <class Container,class Predicate>
void remove_if_stable(Container &cont,Predicate pred)
{
    cont.erase(std::remove_copy_if(cont.begin(),cont.end(),pred),cont.end());
}

template <class Container> inline
void trim(Container &cont,std::size_t size)
{
    if (size < cont.size())
        cont.resize(size); //        cont.erase(cont.begin()+size,cont.end());
}

template <class Container> inline
void grow(Container &cont,std::size_t size,const typename Container::value_type &default_value=typename Container::value_type())
{
    std::size_t contsz=cont.size();
    if (size > contsz)
        cont.resize(size,default_value); //       cont.insert(cont.end()+size,size-contsz,default_value);
}

template <class Container,class Iter> inline
void append(Container &cont,Iter beg,Iter end)
{
    cont.insert(cont.end(),beg,end);
}

// Order e.g. less_typeless
template <class Cont,class Order> inline
void sort(Cont &cont,Order order)
{
    std::sort(cont.begin(),cont.end(),order);
}

// Order e.g. less_typeless
template <class Cont,class Order> inline
typename Cont::iterator sortFirstN(Cont &cont,std::size_t N,Order order)
{
  typename Cont::iterator mid=std::min(cont.begin()+N,cont.end());
  std::partial_sort(cont.begin(),mid,cont.end(),order);
  return mid;
}


template <class Cont> inline
void sort(Cont &cont)
{
    std::sort(cont.begin(),cont.end());
}

template <class Cont> inline
void unique(Cont &cont)
{
    cont.erase(
        std::unique(cont.begin(),cont.end()),
        cont.end());
}

template <class Cont,class Order> inline
void unique(Cont &cont,Order order)
{
    cont.erase(
        std::unique(cont.begin(),cont.end(),order),
        cont.end());
}

template <class Cont,class Order> inline
void sort_unique(Cont &cont,Order order)
{
    sort(cont,order);
    unique(cont,order);
}

template <class Cont,class Func> inline
void for_each(Cont &cont,Func func)
{
    std::for_each(cont.begin(),cont.end(),func);
}

template <class Cont> inline
void sort_unique(Cont &cont)
{
    sort(cont);
    unique(cont);
}

template <class Ck,class Cv,class Cr> inline
void zip_lists_to_pairlist(const Ck &K,const Cv &V,Cr &result)
{
    result.clear();
    typename Ck::const_iterator ik=K.begin(),ek=K.end();
    typename Cv::const_iterator iv=V.begin(),ev=V.end();
    for(;ik<ek && iv<ev;++ik,++iv)
        result.push_back(typename Cr::value_type(*ik,*iv));
    assert(ik==ek && iv==ev);
}

template <class Ck,class Cv,class Cr> inline
std::vector<std::pair<typename Ck::value_type,typename Cv::value_type> > zip_lists(const Ck &K,const Cv &V)
{
    std::vector<std::pair<typename Ck::value_type,typename Cv::value_type> > result;
    zip_lists_to_pairlist(K,V,result);
    return result;
}

template <bool descending=false>
struct compare_fabs
{
    template <class T1,class T2>
    bool operator()(const T1&a,const T2&b) const
    {
        return (descending) ?
            fabs(b) < fabs(a)
            :
            fabs(a) < fabs(b);
    }
};

template <class compare=compare_fabs<true> >
struct select1st_compare : public compare
{
    template <class T1,class T2>
    bool operator()(const T1&a,const T2&b) const
    {
        return compare::operator()(a.first,b.first);
    }
};


template <class FloatVec> inline
typename FloatVec::value_type
norm(const FloatVec &vec)
{
    typedef typename FloatVec::const_iterator Iter;
    typedef typename FloatVec::value_type Float;
    Float sumsq=0;
    for (Iter i=vec.begin(),e=vec.end();i!=e;++i) {
        Float v=*i;
        sumsq+=v*v;
    }
    return sqrt(sumsq);
}

template <class FloatVec,class Scalar> inline
void
scale(FloatVec &vec,const Scalar &scale)
{
    typedef typename FloatVec::iterator Iter;
    for (Iter i=vec.begin(),e=vec.end();i!=e;++i)
        *i*=scale;
}

template <class FloatVec,class Scalar> inline
void
unscale(FloatVec &vec,const Scalar &scale)
{
    typedef typename FloatVec::iterator Iter;
    for (Iter i=vec.begin(),e=vec.end();i!=e;++i)
        *i/=scale;
}

template <class FloatVec> inline
typename FloatVec::value_type
normalize(FloatVec &vec)
{
    typedef typename FloatVec::iterator Iter;
    typedef typename FloatVec::value_type Float;
    Float n=norm(vec);
    if (n > 0)
        unscale(vec,n);
    return n;
}

template <class FloatVec,class V2> inline
typename FloatVec::value_type
norm_of_inner_product(const FloatVec &vec,const V2 &v2)
{
    typedef typename FloatVec::const_iterator Iter;
    typedef typename V2::const_iterator Iter2;
    typedef typename FloatVec::value_type Float;
    Float sumsq=0;
    Iter2 i2=v2.begin();
    assert(vec.size()==v2.size());
    for (Iter i=vec.begin(),e=vec.end();i!=e;++i,++i2) {
        Float v=*i * *i2;
        sumsq+=v*v;
    }
    return sqrt(sumsq);
}

template <class V1,class V2,class Vout> inline
void inner_product(const V1 &v1,const V2 &v2,Vout &vout)
{
    vout.clear();
    vout.reserve(v1.size());
    assert(v1.size()==v2.size());
    typename V1::const_iterator iv1=v1.begin(),ev1=v1.end();
    typename V2::const_iterator iv2=v2.begin();//,ev=V.end();
    for(;iv1<ev1;++iv1,++iv2)
        vout.push_back(*iv1 * *iv2);
}

template <class V1,class V2,class Vout> inline
std::vector<typename V1::value_type> inner_product(const V1 &v1,const V2 &v2)
{
    std::vector<typename V1::value_type> ret;
    inner_product(v1,v2,ret);
    return ret;
}

// I hope you have an efficient swap :)
template <class Container> inline
void compact(Container &c) {
    Container(c).swap(c); // copy (-> temp), swap, temp destructed = old destructed
}

template <class c,class t,class a> inline
void compact(std::basic_string<c,t,a> &s) {
    s.reserve(0); // suggested by std to shrink size to actual amount used.
}

#include <string>

inline std::string subspan(const std::string &s,std::string::size_type begin,std::string::size_type end)
{
    return std::string(s,begin,end-begin);
}

template <typename Container> inline
void stringtok (Container &container, std::string const &in, const char * const delimiters = " \t\n")
{
    const std::string::size_type len = in.length();
    std::string::size_type i = 0;
    while ( i < len )
    {
        // eat leading whitespace
        i = in.find_first_not_of (delimiters, i);
        if (i == std::string::npos)
            return;

        // find the end of the token
        std::string::size_type j = in.find_first_of (delimiters, i);

        // push token
        if (j == std::string::npos) {
            container.push_back (in.substr(i));
            return;
        } else
            container.push_back (in.substr(i, j-i));
        i = j + 1;
    }
}


// requirement: P::result_type value semantics, default initializes to boolean false (operator !), and P itself copyable (value)
// can then pass finder<P>(P()) to enumerate() just like find(beg,end,P())
template <class P>
struct finder
{
    typedef typename P::result_type result_type;
    typedef finder<P> Self;

    P pred;
    result_type ret;

    finder(const P &pred_): pred(pred_),ret() {}
    finder(const Self &o) : pred(o.pred),ret() {}

    void operator()(const result_type& u)
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


/*
template <typename T,size_t n>
struct fixed_single_allocator {
//    enum make_not_anon_13 { size = n };
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
  new(to) P(from);
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
}

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
//        DBP2(period,left);
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
    periodic_wrapper(unsigned period_=0,const Imp &imp_=Imp()) : Imp(imp_),period(period_) {}
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

template <class I,class F>
bool all(I i,I end,F const& f) {
  // return std::find_if(i,end,std::not1(f))==end
  for(;i!=end;++i)
    if (!f(*i)) return false;
  return true;
}

template <class I,class F>
bool all(I const& c,F const& f) {
  return all(c.begin(),c.end(),f);
}


template <class I,class F>
bool exists(I i,I end,F const& f) {
  // return std::find_if(i,end,f)!=end
  for(;i!=end;++i)
    if (f(*i)) return true;
  return false;
}

template <class I,class F>
bool exists(I const& c,F const& f) {
  return exists(c.begin(),c.end(),f);
}

}

#endif
