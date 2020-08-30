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
#ifndef GRAEHL_SHARED__FTOA_HPP
#define GRAEHL_SHARED__FTOA_HPP
/**
   locale independent float->string; (optionally round-trip capable by using
   normally insignificant decimal digits). snprintf probably is equally fast
   but is affected by system locale.

   in C++17 std::to_chars will be ideal instead of this
*/

// http://www.exploringbinary.com/the-shortest-decimal-string-that-round-trips-may-not-be-the-nearest/
// - could be useful if the number is never actually computed with as decimal

#ifndef GRAEHL_FTOA_ROUNDTRIP
// 1 => http://www.exploringbinary.com/number-of-digits-required-for-round-trip-conversions/
#define GRAEHL_FTOA_ROUNDTRIP 0
#endif

#ifndef GRAEHL_FTOA_USE_SPRINTF
// 1 => output depends on system locale
#define GRAEHL_FTOA_USE_SPRINTF 0
#endif

#ifndef GRAEHL_DECIMAL_FOR_WHOLE
/*
   GRAEHL_DECIMAL_FOR_WHOLE=0 => ftos(123) = "123"
   GRAEHL_DECIMAL_FOR_WHOLE=1 => ftos(123) = "123"
   GRAEHL_DECIMAL_FOR_WHOLE=2 => ftos(123) = "123."

   GRAEHL_DECIMAL_FOR_WHOLE=0 => ftos(.01) = ".01"
   GRAEHL_DECIMAL_FOR_WHOLE=1 => ftos(.01) = "0.01"
   GRAEHL_DECIMAL_FOR_WHOLE=2 => ftos(.01) = "0.01"

   ftos(0) = "0" (no matter what)
*/
#define GRAEHL_DECIMAL_FOR_WHOLE 1
#endif

#include <graehl/shared/itoa.hpp>
#include <graehl/shared/nan.hpp>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <limits>
#include <assert.h>

namespace graehl {


template <class Float>
struct ftoa_traits {};

#define GRAEHL_DEFINE_FTOA_TRAITS(FLOATT, INTT, sigd, roundtripd, small, large, used, P10)                 \
  template <>                                                                                              \
  struct ftoa_traits<FLOATT> {                                                                             \
    typedef std::INTT int_t;                                                                               \
    typedef std::u##INTT uint_t;                                                                           \
    typedef FLOATT float_t;                                                                                \
    enum {                                                                                                 \
      digits10 = std::numeric_limits<INTT>::digits10,                                                      \
      chars_block = P10,                                                                                   \
      usedig = used,                                                                                       \
      sigdig = sigd,                                                                                       \
      roundtripdig = roundtripd,                                                                           \
      bufsize = roundtripdig + 7                                                                           \
    };                                                                                                     \
    static constexpr double pow10_block = 1e##P10;                                                         \
    static constexpr float_t small_f = small;                                                              \
    static constexpr float_t large_f = large;                                                              \
    static inline int sprintf(char* buf, double f) { return std::sprintf(buf, "%." #used "g", f); }        \
    static inline int sprintf_sci(char* buf, double f) { return std::sprintf(buf, "%." #used "e", f); }    \
    static inline int sprintf_nonsci(char* buf, double f) { return std::sprintf(buf, "%." #used "f", f); } \
    static inline uint_t fracblock(double frac) {                                                          \
      double f = frac * pow10_block;                                                                       \
      uint_t i = (uint_t)f;                                                                                \
      return i;                                                                                            \
    }                                                                                                      \
    static inline uint_t rounded_fracblock(double frac) {                                                  \
      double f = frac * pow10_block;                                                                       \
      uint_t i = (uint_t)(f + .5);                                                                         \
      return i;                                                                                            \
    }                                                                                                      \
    static inline float_t mantexp10(float_t f, int& exp) {                                                 \
      float_t e = std::log10(f);                                                                           \
      float_t ef = std::floor(e);                                                                          \
      exp = ef;                                                                                            \
      return f / std::pow((float_t)10, ef);                                                                \
    }                                                                                                      \
    static inline bool use_sci_abs(float_t fa) { return fa < small || fa > large; }                        \
    static inline bool use_sci(float_t f) { return use_sci_abs(std::fabs(f)); }                            \
  };


#if GRAEHL_FTOA_ROUNDTRIP
#define GRAEHL_DEFINE_FTOA_TRAITS_ROUNDTRIP(FLOATT, INTT, sigd, roundtripd, small, large) \
  GRAEHL_DEFINE_FTOA_TRAITS(FLOATT, INTT, sigd, roundtripd, small, large, roundtripd, roundtripd)
#else
#define GRAEHL_DEFINE_FTOA_TRAITS_ROUNDTRIP(FLOATT, INTT, sigd, roundtripd, small, large) \
  GRAEHL_DEFINE_FTOA_TRAITS(FLOATT, INTT, sigd, roundtripd, small, large, sigd, sigd + 3)
#endif

// 10^22 = 0x1.0f0cf064dd592p73 is the largest exactly representable power of 10 in the binary64 format.
GRAEHL_DEFINE_FTOA_TRAITS_ROUNDTRIP(double, int64_t, 15, 17, 1e-2, 1e22)
/// we only have 1e9 (9 decimal places) in int32.
GRAEHL_DEFINE_FTOA_TRAITS_ROUNDTRIP(float, int32_t, 6, 9, 1e-2, 1e8)

/// prepend_ methods prepend and return new cursor. null terminate later yourself if desired

/// possibly empty string for ~0 (no sci notation fallback).  left padded with the right number of 0s
/// (tricky). [ret,p) are the digits.
template <class F>
char* prepend_pos_frac_digits(char* p, F f) {
  typedef ftoa_traits<F> FT;
  typename FT::uint_t i = FT::rounded_fracblock(f);
  if (i > 0) {
    unsigned n_skipped;
    char* d = utoa_drop_trailing_0(p, i, n_skipped);
    char* b = p - FT::chars_block + n_skipped;
    left_pad(b, d, '0');
    return b;
  } else {
    return p;
  }
}

/// '0' right-padded, nul terminated, return position of nul.  [p,ret) are the digits
template <class F>
char* append_pos_frac_digits(char* p, F f) {
  if (f == 0) {
    *p++ = '0';
    return p;
  }
  typedef ftoa_traits<F> FT;
  typename FT::uint_t i = FT::rounded_fracblock(f);
  if (i > 0) {
    char* e = p + FT::chars_block;
    utoa_left_pad(p, e, i, '0');
    *e = 0;
    return e;
  } else {
    *p = 0;
    return p;
  }
}

template <class F>
inline char* prepend_pos_frac(char* p, F f, char decimal = '.') {
  if (f == 0) {
    *--p = '0';
    return p;
  }
  p = prepend_pos_frac_digits(p, f);
  *--p = decimal;
  if (GRAEHL_DECIMAL_FOR_WHOLE > 0)
    *--p = '0';
  return p;
}

template <class F>
inline char* append_pos_frac(char* p, F f, char decimal = '.') {
  if (GRAEHL_DECIMAL_FOR_WHOLE > 0)
    *p++ = '0';
  *p++ = decimal;
  return append_pos_frac_digits(p, f);
}

template <class F>
inline char* prepend_frac(char* p, F f, bool positive_sign = false, char decimal = '.') {
  if (f == 0)
    *--p = '0';
  else if (f < 0) {
    p = prepend_pos_frac(p, -f, decimal);
    *--p = '-';
  } else {
    p = prepend_pos_frac(p, f, decimal);
    if (positive_sign)
      *--p = '+';
  }
  return p;
}

template <class F>
inline char* append_sign(char* p, F f, bool positive_sign = false) {
  if (f < 0) {
    *p++ = '-';
  } else if (positive_sign)
    *p++ = '+';
  return p;
}

template <class F>
inline char* append_frac(char* p, F f, bool positive_sign = false, char decimal = '.') {
  if (f == 0) {
    *p++ = '0';
    return p;
  } else if (f < 0) {
    *p++ = '-';
    return append_pos_frac(p, -f, decimal);
  }
  if (positive_sign) {
    *p++ = '+';
    return append_pos_frac(p, f, decimal);
  }
}

template <class F>
char* prepend_pos_nonsci(char* p, F f, char decimal = '.') {
  typedef ftoa_traits<F> FT;
  typedef typename FT::uint_t uint_t;
  uint_t u = f;
  F frac = f - u;
  // could use std::modf, which returns negative frac part if f is negative; but
  // this fn is only for positive f anyway
  if (frac == 0) {
    if (GRAEHL_DECIMAL_FOR_WHOLE > 1)
      *--p = decimal;
  } else {
    p = prepend_pos_frac_digits(p, frac);
    *--p = decimal;
  }
  if (u == 0) {
    if (GRAEHL_DECIMAL_FOR_WHOLE > 0)
      *--p = '0';
  } else
    p = utoa(p, u);
  return p;
}

template <class F>
inline char* prepend_pos_sci(char* p, F f, char decimal = '.', bool positive_sign_exp = false) {
  typedef ftoa_traits<F> FT;
  int e10;
  F mant = FT::mantexp10(f, e10);
  if (mant >= 10.) {
    ++e10;
    mant *= .1;
  } else if (mant < 1.) {
    --e10;
    mant *= 10;
  }
  p = itoa(p, e10, positive_sign_exp);
  *--p = 'e';
  return prepend_pos_nonsci(p, mant, decimal);
}

/** will switch to sci notation if integer part is too big for the int type. but
  for very small values, will simply display 0; call prepend_pos_sci for small
  values */
template <class F>
char* prepend_pos_preferably_nonsci(char* p, F f, char decimal = '.') {
  return f > std::numeric_limits<uint_t>::max() ? return prepend_pos_sci(p, f, decimal)
                                                : prepend_pos_nonsci(p, f, decimal);
}

// modify p; return true if handled
template <class F>
inline bool prepend_0_etc(char*& p, F f, bool positive_sign = false) {
  if (f == 0) {
    *--p = '0';
    return true;
  }
  if (is_nan(f)) {
    p -= 3;
    p[0] = 'N';
    p[1] = 'A';
    p[2] = 'N';
    return true;
  }
  if (is_pos_inf(f)) {
    p -= 3;
    p[0] = 'I';
    p[1] = 'N';
    p[2] = 'F';
    if (positive_sign)
      *--p = '+';
    return true;
  }
  if (is_neg_inf(f)) {
    p -= 4;
    p[0] = '-';
    p[1] = 'I';
    p[2] = 'N';
    p[3] = 'F';
    return true;
  }
  return false;
}

template <class F>
inline char* prepend_nonsci(char* p, F f, bool positive_sign = false, char decimal = '.') {
  if (prepend_0_etc(p, f, positive_sign))
    return p;
  if (f < 0) {
    p = prepend_pos_nonsci(p, -f, decimal);
    *--p = '-';
  } else {
    p = prepend_pos_nonsci(p, f, decimal);
    if (positive_sign)
      *--p = '+';
  }
  return p;
}

template <class F>
inline char* prepend_sci(char* p, F f, char decimal = '.', bool positive_sign_mant = false,
                         bool positive_sign_exp = false) {
  if (prepend_0_etc(p, f, positive_sign_mant))
    return p;
  if (f == 0)
    *--p = '0';
  else if (f < 0) {
    p = prepend_pos_sci(p, -f, decimal, positive_sign_exp);
    *--p = '-';
  } else {
    p = prepend_pos_sci(p, f, decimal, positive_sign_exp);
    if (positive_sign_mant)
      *--p = '+';
  }
  return p;
}

template <class F>
inline char* prepend_ftoa(char* p, F f, char decimal = '.') {
  typedef ftoa_traits<F> FT;
  return FT::use_sci(f) ? prepend_sci(p, f, decimal) : prepend_nonsci(p, f, decimal);
}

template <class F>
inline std::string ftos_prepend(F f, char decimal = '.') {
  typedef ftoa_traits<F> FT;
  char buf[FT::bufsize];
  char* end = buf + FT::bufsize;
  return std::string(prepend_ftoa(end, f, decimal), end);
}

template <class F>
inline std::string ftos(F f, char decimal = '.') {
  return ftos_prepend(f, decimal);
}


} // namespace graehl


#ifdef GRAEHL_FTOA_SAMPLE
#include <cstdio>
#include <iostream>
#include <sstream>
using namespace std;
using namespace graehl;
int main(int argc, char* argv[]) {
  cerr << "\n";
  for (int i = 1; i < argc; ++i)
    cerr << argv[i] << "\n";
  cerr << "\n\n";
  for (int ii = 1; ii < argc; ++ii) {
    double d;
    sscanf(argv[ii], "%lf", &d);
    float f = d;
    cerr << d << " ->float= " << f << endl;
    cerr << f << " ->ftoa(f)= " << ftos(f) << " "
         << " ->ftoa(d)=" << ftos(d);
    cerr << " ->ftoa_pre(f)= " << ftos(f) << " ->ftoa_pre(d)= " << ftos(d);
    cerr << " "
         << " ->ftoa_post(f)=" << ftos_append(f);
    cerr << " "
         << " ->ftoa_post(d)=" << ftos_append(d);
    const int bufsz = 500;
    char buf[bufsz];
    char* p = buf + bufsz;
    {
      float intp;
      float frac = fabs(modf(f, &intp));
      cerr << endl;
      cerr << "int=" << intp << " frac=" << frac;
      cerr << " pre_pos_frac=" << string(prepend_pos_frac(p, frac), p);
      cerr << " pre_frac=" << string(prepend_frac(p, frac, true), p);
      cerr << " pre_sci=" << string(prepend_sci(p, f), p);
      cerr << " pre_nonsci=" << string(prepend_nonsci(p, f), p);
      cerr << "\n\n";
    }
  }
  return 0;
}
#endif

#ifdef GRAEHL_TEST
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/test.hpp>

namespace graehl {

template <class F>
inline void testFtoa(F x, F scale, F max = 1e30, unsigned n = 10000) {
  for (unsigned i = 0; i < n && x <= max; x *= scale) {
    F y, ny;
    string_to(ftos(x), y);
    string_to(ftos(-x), ny);
#if GRAEHL_FTOA_ROUNDTRIP
    BOOST_CHECK_EQUAL(x, y);
    BOOST_CHECK_EQUAL(-x, ny);
#else
    BOOST_CHECK_CLOSE(x, y, 1e-5);
    BOOST_CHECK_CLOSE(-x, ny, 1e-5);
#endif
  }
}

BOOST_AUTO_TEST_CASE(TestFtoaDouble) {
  BOOST_CHECK_EQUAL(ftos(0.f), "0");
  BOOST_CHECK_EQUAL(ftos(0.), "0");
  testFtoa(1e-250, 3.143848437, 1e300);
  testFtoa(1.3188437e-10, 5.3e12, 1e300);
  testFtoa((float)1e-25f, 3.1415f, 1e30f);
  testFtoa((float)1.318843e-10f, 5.3e3f, 1e30f);
}
#endif


}


#endif
