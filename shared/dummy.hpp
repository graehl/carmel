#ifndef GRAEHL_SHARED__DUMMY_HPP
#define GRAEHL_SHARED__DUMMY_HPP

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

template <class C>
struct dummy {
  static const C &var();
};


template <class C>
const C& dummy<C>::var() {
  static C var;
  return var;
}

#ifdef GRAEHL_TEST

BOOST_AUTO_TEST_CASE( TEST_dummy )
{
  BOOST_CHECK(dummy<int>::var() == 0);
}
#endif

}

#endif
