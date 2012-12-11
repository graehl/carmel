#ifndef GRAEHL__SHARED__BYREF_HPP
#define GRAEHL__SHARED__BYREF_HPP
// when you write a template that takes an argument by value, pass a ref(obj) if you don't want a copy of obj made.  tries to autoconvert back to obj's type but can't do that for operators, so e.g. deref(f)(arg) is necessary in your template function.
// originally contained my own wrap-reference-as-value class until I discovered Boost's.


#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

//boost::ref and boost::cref that return instances of boost::reference_wrapper<T>
// boost::unwrap_reference<T>::type = T
// boost::unwrap_reference<boost::reference_wrapper<T> > = T

#include <boost/ref.hpp>

#include <graehl/shared/dummy.hpp>



namespace graehl {


template <class C>
struct dummy<boost::reference_wrapper<C> > {
  //  static C dummy_imp;
  static boost::reference_wrapper<C> var();
};

template <class C>
boost::reference_wrapper<C> dummy<boost::reference_wrapper<C> >::var() {
  static boost::reference_wrapper<C> var(*(C*)NULL);
  return var;
}
/*
template <class C>
struct ByRef {
  typedef C type;
  operator C&() const { return *c; }
  explicit ByRef(C &c_) : c(&c_) {}
  ByRef(ByRef<C> &b) : c(b.c) {}
  private:
    C *c;
};
*/

//#define ByRef boost::reference_wrapper
/*
template <class C>
struct ByRef : public boost::reference_wrapper<C> {
  explicit ByRef(C &c_) : boost::reference_wrapper<C>(c_) {}
};
*/

template <class T> inline
typename boost::unwrap_reference<T>::type &
deref(T& t) {
  return t;
}

template <class T> inline
const typename boost::unwrap_reference<T>::type &
deref(const T& t) {
  return t;
}
  //return *const_cast<boost::unwrap_reference<T>::type *>&(t);


// for containers of containers where you want to visit every element
template <class M,class F>
void nested_enumerate(const M& m,F f) {
    typedef typename M::value_type Inner;
    for (typename M::const_iterator i=m.begin();i!=m.end();++i)
        for (typename Inner::const_iterator j=i->begin();j!=i->end();++j)
            deref(f)(*j);
}

template <class M,class F>
void nested_enumerate(M& m,F f) {
    typedef typename M::value_type Inner;
    for (typename M::iterator i=m.begin();i!=m.end();++i)
        for (typename Inner::iterator j=i->begin();j!=i->end();++j)
            deref(f)(*j);
}


template <class M,class F>
void enumerate(const M& m,F f) {
  for (typename M::const_iterator i=m.begin();i!=m.end();++i)
    deref(f)(*i);
}

template <class M,class F>
void enumerate(M& m,F f) {
  for (typename M::iterator i=m.begin();i!=m.end();++i)
    deref(f)(*i);
}

}

namespace std {

template<class R>
struct equal_to<boost::reference_wrapper<R> >
{
    typedef boost::reference_wrapper<R> arg_type;
    typedef arg_type first_argument_type;
    typedef arg_type second_argument_type;
    typedef bool result_type;

    bool operator()(arg_type const& r1,arg_type const& r2) const
    {
        return (R const &)r1 == (R const &)r2;
    }

};

}

namespace boost {

template<class R>
struct hash;


template<class R>
struct hash<boost::reference_wrapper<R> >
{
    typedef boost::reference_wrapper<R> arg_type;
    typedef std::size_t result_type;

    result_type operator()(arg_type const& r) const
    {
        return hash_value((R const&)r);
    }
};

}


#ifdef GRAEHL_TEST_MAIN
namespace byref_test{


template<class C>
void f(C c) {
    graehl::deref(c)=1;
}

void g(int &p) {
  p=2;
}

template<class C>
void h(C c) {
  g(c);
}
}


BOOST_AUTO_TEST_CASE( TEST_byref )
{
    using namespace byref_test;
  int t=0;
  f(t);
  BOOST_CHECK(t==0);
  f(boost::ref(t));
  BOOST_CHECK(t==1);
  h(t);
  BOOST_CHECK(t==1);
  h(boost::ref(t));
  BOOST_CHECK(t==2);
}
#endif


#endif
