#ifndef _BYREF_HPP
#define _BYREF_HPP

//boost::ref and boost::cref that return instances of boost::reference_wrapper<T>
// boost::unwrap_reference<T>::type = T
// boost::unwrap_reference<boost::reference_wrapper<T> > = T

#include <boost/ref.hpp>

#include "dummy.hpp"

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

#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
template<class C>
void f(C c) {
  deref(c)=1;
}

void g(int &p) {
  p=2;
}

template<class C>
void h(C c) {
  g(c);
}

BOOST_AUTO_UNIT_TEST( TEST_byref )
{
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
