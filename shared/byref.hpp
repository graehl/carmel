#ifndef _BYREF_HPP
#define _BYREF_HPP

//boost::ref and boost::cref that return instances of boost::reference_wrapper<T>
// boost::unwrap_reference<T>::type = T
// boost::unwrap_reference<boost::reference_wrapper<T> > = T

#include <boost/ref.hpp>

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
template <class C>
struct ByRef : public boost::reference_wrapper<C> {
  explicit ByRef(C &c_) : boost::reference_wrapper<C>(c_) {}
};


#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
template<class C>
void f(C c) {
  c=1;
}

BOOST_AUTO_UNIT_TEST( byref )
{
  int t=0;
  f(ByRef<int>(t));
  BOOST_CHECK(t==1);
}
#endif

#endif
