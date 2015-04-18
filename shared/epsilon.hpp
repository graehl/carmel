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

    floats considered very nearly equal when they (relatively) differ by some small epsilon.
*/

#ifndef GRAEHL__SHARED__EPSILON_HPP
#define GRAEHL__SHARED__EPSILON_HPP
#pragma once

#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <boost/cstdint.hpp>

#ifndef GRAEHL_EPSILON
/**
   compare to 1e-5 (FLT_EPSILON) 1e-9 (DBL_EPSILON) - Difference between 1 and
   the least value greater than 1 that is representable.

   for simplicity we use the same difference for both
*/
#define GRAEHL_EPSILON 2e-5
#endif

#ifndef GRAEHL_DEFAULT_IEEE_APART
/// for within_epsilon_or_(floats|doubles|ieee)_apart(a, b, epsilon): check approx
/// equality by the number of representable IEEE float/double between two
/// numbers
#define GRAEHL_DEFAULT_IEEE_APART 8
// for floats, this is about 1 in a million
#endif

namespace graehl {

#ifndef ONE_PLUS_EPSILON
#ifndef EPSILON
static const double EPSILON = GRAEHL_EPSILON;
#endif
static const double ONE_PLUS_EPSILON = 1 + EPSILON;
#endif

//#define ONE_PLUS_EPSILON (1+EPSILON)


/*
  The simple solution like abs(f1-f2) <= e does not work for very small or very big values. This
  floating-point comparison algorithm is based on the more confident solution presented by Knuth in [1]. For a
  given floating point values u and v and a tolerance e:

  | u - v | <= e * |u| and | u - v | <= e * |v|
  defines a "very close with tolerance e" relationship between u and v
  (1)

  | u - v | <= e * |u| or | u - v | <= e * |v|
  defines a "close enough with tolerance e" relationship between u and v
  (2)

  Both relationships are commutative but are not transitive. The relationship defined by inequations (1) is
  stronger that the relationship defined by inequations (2) (i.e. (1) => (2) ). Because of the multiplication
  in the right side of inequations, that could cause an unwanted underflow condition, the implementation is
  using modified version of the inequations (1) and (2) where all underflow, overflow conditions could be
  guarded safely:

  | u - v | / |u| <= e and | u - v | / |v| <= e
  | u - v | / |u| <= e or | u - v | / |v| <= e
  (1`)
  (2`)
*/


// intent: if you want to be conservative about an assert of a<b, test a<(slightly smaller b)
// if you want a<=b to succeed when a is == b but there were rounding errors so that a+epsilon=b, test
// a<(slightly larger b)
template <class Float>
inline Float slightly_larger(Float target) {
  return target * ONE_PLUS_EPSILON;
}

template <class Float>
inline Float slightly_smaller(Float target) {
  return target * (1. / ONE_PLUS_EPSILON);
}

// note, more meaningful tests exist for values near 0, see Knuth
// (but for logs, near 0 should be absolute-compared)
inline bool same_within_abs_epsilon(double a, double b, double epsilon = EPSILON) {
  return std::fabs(a - b) < epsilon;
}

inline bool close_by_first(double a, double b, double epsilon = EPSILON) {
  return std::fabs(a - b) <= epsilon * std::fabs(a);
}

/**
   \return a==b (approximately), symmetric (strict)
*/
inline bool very_close(double a, double b, double epsilon = EPSILON) {
  using std::fabs;
  double diff = fabs(a - b);
  return diff <= epsilon * fabs(a) && diff <= epsilon * fabs(b);
}

/**
   \return a==b (approximately), symmetric (looser)
*/
inline bool close_enough(double a, double b, double epsilon = EPSILON) {
  using std::fabs;
  double diff = fabs(a - b);
  return diff <= epsilon * fabs(a) || diff <= epsilon * fabs(b);
}

/**
   \return a==b (approximately)

   epsilon is relative (if a or b is larger than min_scale) or else absolute
   (using epsilon*min_scale)

*/
inline bool close_enough_min_scale(double a, double b, double epsilon = EPSILON, double min_scale = 1.) {
  using std::fabs;
  double diff = fabs(a - b);
  double scale = std::max(min_scale, std::max(fabs(a), fabs(b)));
  return diff <= epsilon * scale;
}

inline bool sign_ieee(boost::uint32_t f) {
  return f >> 31;
}

inline bool sign_ieee(float f) {
  return sign_ieee(reinterpret_cast<boost::uint32_t const&>(f));
}

inline bool sign_ieee(boost::uint64_t f) {
  return f >> 63;
}

inline bool sign_ieee(double f) {
  return sign_ieee(reinterpret_cast<boost::uint64_t const&>(f));
}

/**
   \return absolute distance between a and b. unlike std::abs(a-b), this is correct

   (also works for signed cast to unsigned)
*/
inline boost::uint32_t unsigned_distance(boost::uint32_t a, boost::uint32_t b) {
  return a < b ? b - a : a - b;
}

/**
   \return absolute distance between a and b. unlike std::abs(a-b), this is correct

   (also works for signed cast to unsigned)
*/
inline boost::uint64_t unsigned_distance(boost::uint64_t a, boost::uint64_t b) {
  return a < b ? b - a : a - b;
}

inline double next_representible_double(double f) {
  ++reinterpret_cast<boost::uint64_t&>(f);
  return f;
}

inline float next_representible_float(float f) {
  ++reinterpret_cast<boost::uint32_t&>(f);
  return f;
}

/**
   \return # of ieee floats between a and b, which must have the same sign

   this works because same-signed IEEE float/double have the same < order as the
   same-sized int (except perhaps for nan)

   see http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
*/
inline boost::int32_t ieee_floats_apart(float a, float b) {
  return unsigned_distance(reinterpret_cast<boost::uint32_t const&>(a),
                           reinterpret_cast<boost::uint32_t const&>(b));
}

/**
   \return # of ieee doubles between a and b, which must have the same sign

   see http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
*/
inline boost::int64_t ieee_doubles_apart(double a, double b) {
  return unsigned_distance(reinterpret_cast<boost::uint64_t const&>(a),
                           reinterpret_cast<boost::uint64_t const&>(b));
}

inline boost::int64_t ieee_apart(float a, float b) {
  return ieee_floats_apart(a, b);
}

inline boost::int64_t ieee_apart(double a, double b) {
  return ieee_doubles_apart(a, b);
}

/**
   \return a==b or within max_floats_apart for ieee floats. a and b should have the same sign.

   (unlike regular floating point a==b, compares non-numbers nan, +inf, etc. equal)

   the absolute epsilon is useful because there are millions of representable
   floats between GRAEHL_EPSILON and 0 (the denormals)

   max_floats_apart allows for slight relative differences when the scale of the
   numbers compared grows too large for epsilon (on the order of 1e-5 relative
   per max_floats_apart)

   see http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
*/
inline bool few_ieee_floats_apart(float a, float b,
                                  boost::int32_t max_floats_apart = GRAEHL_DEFAULT_IEEE_APART) {
  return sign_ieee(a) == sign_ieee(b) ? ieee_floats_apart(a, b) <= max_floats_apart : a == b;
  // implementation note: we could instead check a==b first, but by checking sign
  // using integral ops, then floats_apart using integral ops, in the common case
  // where signs match, it should be slightly more efficient (memory contents
  // need not be loaded to FPU). of course, if the floats were already in FPU,
  // maybe it would be better to use a similar relative difference check using
  // floating point ops only, e.g. close_enough or close_enough_min_scale
}

/**
   note: there are many adjacent doubles per adjacent float - 52 sig binary
   digits vs 23? so half a billion times as many. therefore these methods have
   different names
*/

inline bool few_ieee_doubles_apart(double a, double b,
                                   boost::int64_t max_doubles_apart = GRAEHL_DEFAULT_IEEE_APART) {
  return sign_ieee(a) == sign_ieee(b) ? ieee_doubles_apart(a, b) <= max_doubles_apart : a == b;
}

inline bool few_ieee_apart(float a, float b, boost::int32_t max_floats_apart = GRAEHL_DEFAULT_IEEE_APART) {
  return few_ieee_floats_apart(a, b, max_floats_apart);
}

inline bool few_ieee_apart(double a, double b, boost::int64_t max_doubles_apart = GRAEHL_DEFAULT_IEEE_APART) {
  return few_ieee_doubles_apart(a, b, max_doubles_apart);
}


/**
   \return a==b (within +- epsilon, absolute).
*/
template <class Float>
inline bool within_epsilon(Float a, Float b, Float epsilon = (float)GRAEHL_EPSILON) {
  return std::fabs(a - b) <= epsilon;
}

/**
   return a==b (within +- epsilon) or if a is no more than max_floats_apart
   representable floats away from b
*/
inline bool within_epsilon_or_floats_apart(float a, float b, float epsilon = (float)GRAEHL_EPSILON,
                                           boost::int32_t max_floats_apart = GRAEHL_DEFAULT_IEEE_APART) {
  return std::fabs(a - b) <= epsilon
         || (sign_ieee(a) == sign_ieee(b) && few_ieee_floats_apart(a, b, max_floats_apart));
}

/**
   return a==b (within +- epsilon) or if a is no more than max_floats_apart
   representable floats away from b
*/
inline bool within_epsilon_or_doubles_apart(double a, double b, double epsilon = (double)GRAEHL_EPSILON,
                                            boost::int64_t max_doubles_apart = GRAEHL_DEFAULT_IEEE_APART) {
  return std::fabs(a - b) <= epsilon
         || (sign_ieee(a) == sign_ieee(b) && few_ieee_doubles_apart(a, b, max_doubles_apart));
}

/**
   return a==b (within +- epsilon) or if a is no more than GRAEHL_DEFAULT_IEEE_APART representable
   floats away from b
*/
inline bool within_epsilon_or_ieee_apart(float a, float b, float epsilon = (float)GRAEHL_EPSILON) {
  return within_epsilon_or_floats_apart(a, b, epsilon, GRAEHL_DEFAULT_IEEE_APART);
}

/**
   return a==b (within +- epsilon) or if a is no more than GRAEHL_DEFAULT_IEEE_APART representable
   doubles away from b. this would be appropriate for ignoring rounding errors
   for computations done entirely with doubles; if you use it for the results on
   computations as floats, it will likely reduce to within_epsilon.
*/
inline bool within_epsilon_or_ieee_apart(double a, double b, double epsilon = (double)GRAEHL_EPSILON) {
  return within_epsilon_or_doubles_apart(a, b, epsilon, GRAEHL_DEFAULT_IEEE_APART);
}
}

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
namespace graehl {
namespace unit_test {

template <class Float>
void testEpsilon(Float f, unsigned n, double max_ieee_apart_per_n = 0.2) {
  Float x = f;
  Float sum = 0;
  for (unsigned i = 0; i < n; ++i) sum += x;
  Float nx = x * n;
  unsigned max_ieee_apart = (unsigned)(max_ieee_apart_per_n * n);
  BOOST_CHECK_MESSAGE(ieee_apart(nx, sum) <= max_ieee_apart, "ieee_apart larger than "
                                                             << max_ieee_apart << ": x=" << x << " n=" << n
                                                             << " x*n=" << nx << " (x+...+x)(n times)=" << sum
                                                             << " ieee_apart=" << ieee_apart(nx, sum));
}

BOOST_AUTO_TEST_CASE(TEST_epsilon) {
  testEpsilon(.2f, 1000);
  testEpsilon(.2, 1000);
  BOOST_CHECK_EQUAL(ieee_apart(1234.05, 1234.06), 43980465111);
  BOOST_CHECK_EQUAL(ieee_apart(1234.05f, 1234.06f), 82);
}


}}

#endif

#endif
