#ifndef DUMMY_HPP
#define DUMMY_HPP

template <class C>
struct dummy {
  static const C &var();
};


template <class C>
const C& dummy<C>::var() {
  static C var;
  return var;
}

#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
BOOST_AUTO_UNIT_TEST( TEST_dummy )
{
  BOOST_CHECK(dummy<int>::var() == 0);
}
#endif

#endif
