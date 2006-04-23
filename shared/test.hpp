// helps me use the Boost Unit Test library
#ifndef TEST_HPP
#define TEST_HPP

#include <string>
#include <sstream>
#include <iostream>

#if defined(GRAEHL__SINGLE_MAIN)
# define TEST_MAIN
#endif
#ifdef TEST_MAIN
# define BOOST_AUTO_TEST_MAIN
#endif

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4267 4535 )
#endif
//included/
//#include <boost/test/unit_test_framework.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/test_tools.hpp>
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

#ifdef GRAEHL__SINGLE_MAIN
  unsigned test_counter::n;
#endif

template <class S,class C> inline
bool test_extract(S &s,C &c,bool whine=true) {
    std::istringstream is(s);
    try {      
        is >> c;
    } catch (std::ios_base::failure &e) {
        if (whine)
            std::cerr << "Exception: " << e.what() << "\n";
        return 0;
    }
    return !is.fail();
}

template <class S,class C> inline
bool test_extract_insert(S &s,C &c,bool whine=true) {
  std::istringstream is(s);
  try {
      is >> c; // string to var
      std::ostringstream o;
      o << c; // var to another string
//  std::ostringstream o2;
//  o2 << s; // string back to another string?  why?
      if (o.str() != s) {
//      DBP(o.str());
//      DBP(o2.str());
          std::cerr << "Output after writing and rereading: "<<o.str()<<std::endl<<" ... didn't match original: " << s << std::endl;
          return 0;
      }
  } catch (std::ios_base::failure &e) {
      if (whine)
          std::cerr << "Exception: " << e.what() << "\n";
      else
          throw e;
      return 0;
  }

  if (is.fail()) {
      std::cerr << "Round trip write then read succeeded, but input stream is flagged as failing\n";
      return 0;
  }
  return 1;
}

  
  template <class C>
  struct expect_visitor 
  {
      unsigned next;
      const C *array_expected;
      expect_visitor(const C * e) : array_expected(e),next(0) {}
      expect_visitor(const expect_visitor &o) : array_expected(o.array_expected),next(o.next) {}
      template <class D>
      bool operator()(const D& d)
      {
          BOOST_CHECK_EQUAL(array_expected[next],d);
          ++next;
          return true;
      }
      unsigned n_visited() const 
      {
          return next;
      }
  };

  template <class C>
  expect_visitor<C> make_expect_visitor(const C *exp) 
  {
      return expect_visitor<C>(exp);
  }
  
      
#define CHECK_EQUAL_STRING(a,b) BOOST_CHECK_EQUAL(std::string(a),std::string(b))
#define CHECK_EXTRACT(s,c) BOOST_CHECK(test_extract((s),(c)))
#define FAIL_EXTRACT(s,c) BOOST_CHECK(!test_extract((s),(c),false))

#endif
