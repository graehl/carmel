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
/** \file

    template traits for null values (faster than boost::optional when applicable)
*/


#ifndef GRAEHL__SHARED__IS_NULL_HPP
#define GRAEHL__SHARED__IS_NULL_HPP
#pragma once

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#define IS_NULL_DEBUG(x) x
#else
#define IS_NULL_DEBUG(x)
#endif

// find is_null, set_null by ADL (Koenig lookup).

#include <graehl/shared/nan.hpp>

//#define FLOAT_NULL HUGE_VALF
//#define DOUBLE_NULL HUGE_VAL
#define FLOAT_NULL float(NAN)
#define DOUBLE_NULL double(NAN)

template <class C>
inline bool is_null(C const& c) {
  return !c;
}

template <class C>
inline void set_null(C& c) {
  c = C();
}

inline bool is_null(float const& f) {
  return GRAEHL_ISNAN(f);
}

inline void set_null(float& f) {
  f = FLOAT_NULL;
}

inline bool is_null(double const& f) {
  return GRAEHL_ISNAN(f);
}

inline void set_null(double& f) {
  f = DOUBLE_NULL;
}

template <class C>
inline bool non_null(C const& c) {
  return !is_null(c);
}

struct as_null {};
// tag for constructors

#define MEMBER_IS_SET_NULL MEMBER_SET_NULL MEMBER_IS_NULL

#define MEMBER_SET_NULL \
  friend bool is_null(self_type const& me) { return me.is_null(); }
#define MEMBER_IS_NULL \
  friend void is_null(self_type& me) { return me.set_null(); }


#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE(TEST_is_null) {
  using namespace std;
  using namespace boost;
  using namespace graehl;

  double d = std::numeric_limits<double>::infinity();
  BOOST_CHECK_EQUAL(is_null(d), false);
  BOOST_CHECK_EQUAL(is_nan(d), false);
  set_null(d);
  BOOST_CHECK_EQUAL(is_null(d), true);
  BOOST_CHECK_EQUAL(is_nan(d), true);

  float f = std::numeric_limits<float>::infinity();
  BOOST_CHECK_EQUAL(is_null(f), false);
  BOOST_CHECK_EQUAL(is_nan(f), false);
  set_null(f);
  BOOST_CHECK_EQUAL(is_null(f), true);
  BOOST_CHECK_EQUAL(is_nan(f), true);
}
#endif

#endif
