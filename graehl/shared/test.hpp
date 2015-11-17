// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// helps me use the Boost Unit Test library
#ifndef GRAEHL__SHARED__TEST_HPP
#define GRAEHL__SHARED__TEST_HPP
#pragma once

#include <string>
#include <sstream>
#include <iostream>

#if defined(GRAEHL__SINGLE_MAIN)
#define GRAEHL_TEST_MAIN
#endif
#ifdef GRAEHL_TEST_MAIN
#define BOOST_TEST_MAIN
#else
#define BOOST_NO_TEST_MAIN
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4267 4535)
#endif

#ifdef GRAEHL_INCLUDED_TEST
#include <boost/test/included/unit_test.hpp>
#else
#include <boost/test/unit_test.hpp>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef BOOST_TEST_MAIN
#ifdef BOOST_NO_EXCEPTIONS
#include <cstdlib>
namespace boost {
void throw_exception(std::exception const& e) {
  std::exit(-1);
}  // user defined
}
#endif

#endif

namespace graehl {

struct test_counter {
  static unsigned n;
  test_counter() { n = 0; }
  void operator()() { ++n; }
  template <class A1>
  void operator()(A1 const& a) {
    ++n;
  }
  template <class A1, class A2>
  void operator()(A1 const& a, A2 const& a2) {
    ++n;
  }
  template <class A1, class A2, class A3>
  void operator()(A1 const& a, A2 const& a2, A3 const& a3) {
    ++n;
  }
};

#ifdef GRAEHL__SINGLE_MAIN
unsigned test_counter::n;
#endif

template <class S, class C>
inline bool test_extract(S& s, C& c, bool whine = true) {
  std::istringstream is(s);
  try {
    is >> c;
  } catch (std::ios_base::failure& e) {
    if (whine) std::cerr << "Exception: " << e.what() << "\n";
    return 0;
  }
  return !is.fail();
}

template <class S, class C>
inline bool test_extract_insert(S& s, C& c, bool whine = true) {
  std::istringstream is(s);
  try {
    is >> c;  // string to var
    std::ostringstream o;
    o << c;  // var to another string
    if (o.str() != s) {
      std::cerr << "Output after writing and rereading: " << o.str() << '\n'
                << " ... didn't match original: " << s << '\n';
      return 0;
    }
  } catch (std::ios_base::failure& e) {
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
struct expect_visitor {
  const C* array_expected;
  unsigned next;
  expect_visitor(const C* e) : array_expected(e), next(0) {}
  expect_visitor(expect_visitor const& o) : array_expected(o.array_expected), next(o.next) {}
  template <class D>
  bool operator()(D const& d) {
    C const& x = array_expected[next++];
    if (x == d)
      return true;
    BOOST_CHECK_EQUAL(x, d);
    ++next;
    return false;
  }
  unsigned n_visited() const { return next; }
};

template <class C>
expect_visitor<C> make_expect_visitor(const C* exp) {
  return expect_visitor<C>(exp);
}


#define CHECK_EQUAL_STRING(a, b) BOOST_CHECK_EQUAL(std::string(a), std::string(b))
#define CHECK_EXTRACT(s, c) BOOST_CHECK(test_extract((s), (c)))
#define FAIL_EXTRACT(s, c) BOOST_CHECK(!test_extract((s), (c), false))

#define EXPECT_TRUE(c) BOOST_CHECK(c);
#define EXPECT_EQ(a, b) BOOST_CHECK_EQUAL(a, b)


}

#endif
