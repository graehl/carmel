#ifndef _TEST_HPP
#define _TEST_HPP

#include <string>
#include <sstream>
#include <iostream>

struct test_counter {
  static unsigned n;
  test_counter() { n=0; }
  void  operator()() { ++n;  }
  template <class A1>
  void  operator()(const A1 &a) { ++n;  }
  template <class A1,class A2>
  void  operator()(const A1 &a,const A2 &a2) { ++n;  }
  template <class A1,class A2,class A3>
  void  operator()(const A1 &a,const A2 &a2,const A3 &a3) { ++n;  }
};

#ifdef MAIN
unsigned test_counter::n;
#define TEST_MAIN
#define BOOST_AUTO_TEST_MAIN
#endif


template <class S,class C> inline
bool test_extract(S &s,C &c) {
  std::istringstream is(s);
  is >> c;
  return !is.fail();
}

//#include "debugprint.hpp"
template <class S,class C> inline
bool test_extract_insert(S &s,C &c) {
  std::istringstream is(s);
  is >> c;
  std::ostringstream o;
  o << c;
  std::ostringstream o2;
  o2 << s;
  if (o.str() != o2.str()) {
//      DBP(o.str());
//      DBP(o2.str());
      std::cerr << "Output after writing and rereading: "<<o2.str()<<std::endl<<" ... didn't match original: " << o.str() << std::endl;
      return 0;
  }
  if (is.fail()) {
      std::cerr << "Round trip write then read succeeded, but input stream is flagged as failing\n";
      return 0;
  }
  return 1;
}


#define CHECK_EXTRACT(s,c) BOOST_CHECK(test_extract((s),(c)))
#define FAIL_EXTRACT(s,c) BOOST_CHECK(!test_extract((s),(c)))

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4267 4535 )
#endif
//included/
//#include <boost/test/unit_test_framework.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#ifdef _MSC_VER
#pragma warning( pop )
#endif
#ifdef BOOST_AUTO_TEST_MAIN
#ifdef BOOST_NO_EXCEPTIONS
#include <cstdlib>
namespace boost
{
  void throw_exception(std::exception const & e) {
        std::exit(-1);
  }// user defined
#endif
#endif
#include "debugprint.hpp"
#endif
