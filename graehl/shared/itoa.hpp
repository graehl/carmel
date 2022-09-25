// Copyright 2014 Jonathan Graehl-http://graehl.org/
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

    fast itoa but supporting unsigned and larger integral types.

    see int_types.hpp signed_for_int<Int>::toa_bufsize (includes 1 extra for trailing 0 and 1 for +- sign)

    notice: some of these fns write the integer backwards (others use a temporary buffer then reverse the
   result)
*/

#ifndef GRAEHL_SHARED__ITOA_H
#define GRAEHL_SHARED__ITOA_H
#pragma once

#include <graehl/shared/append.hpp>
#include <graehl/shared/int_types.hpp>
#include <cassert>
#include <cstring>
#include <limits>
#include <string>


// define this if you're paranoid about converting 0-9 (int) to 0-9 (char) by
// adding to '0', which is safe for ascii, utf8, etc.
#ifndef GRAEHL_ITOA_DIGIT_LOOKUP_TABLE
#define GRAEHL_ITOA_DIGIT_LOOKUP_TABLE 0
#endif

// maybe this is faster than mod because we are already dividing. //TODO:
// benchmark to prove that
#define GRAEHL_ITOA_NDIV10MOD(rem, n) \
  rem = n;                            \
  n /= 10;                            \
  rem -= 10 * n;

namespace graehl {

#ifdef GRAEHL_ITOA_DIGIT_LOOKUP_TABLE
namespace {
char const digits[] = "0123456789";
}
#endif

inline char digit_to_char(int d) {
  return
#ifdef GRAEHL_ITOA_DIGIT_LOOKUP_TABLE
      digits[d];
#else
      '0' + d;
#endif
}

// returns n in string [return, num); *num=0 yourself before calling if you want a c_str. in other words, the
// sequence [ret, buf) contains the written digits
template <class Int>
char* utoa(char* buf, Int n_) {
  typedef typename signed_for_int<Int>::unsigned_t Uint;
  Uint n = n_;
  if (!n) {
    *--buf = '0';
  } else {
    Uint rem;
    // 3digit lookup table, divide by 1000 faster?
    while (n) {
      GRAEHL_ITOA_NDIV10MOD(rem, n);
      *--buf = digit_to_char(rem);
    }
  }
  return buf;
}

// left_pad_0(buf, utoa(buf+bufsz, n)) means that [buf, buf+bufsz) is a left-0 padded seq. of digits. no 0s
// are added if utoa is already past buf (you must have ensured that this is valid memory, naturally)
inline void left_pad(char const* left, char* buf, char pad = '0') {
  while (buf > left) *--buf = pad;
  // return buf;
}

template <class Int>
char* utoa_left_pad(char* buf, char* bufend, Int n, char pad = '0') {
  char* r = utoa(bufend, n);
  assert(buf <= r);
  left_pad(buf, r, pad);
  return buf;
}

// note: 0 -> 0, but otherwise x000000 -> x (x has no trailing 0s). same conditions as utoa; [ret, buf) gives
// the sequence of digits
// useful for floating point fraction output
template <class Uint_>
char* utoa_drop_trailing_0(char* buf, Uint_ n_, unsigned& n_skipped) {
  typedef typename signed_for_int<Uint_>::unsigned_t Uint;
  Uint n = n_;
  n_skipped = 0;
  if (!n) {
    *--buf = '0';
    return buf;
  } else {
    Uint rem;
    while (n) {
      GRAEHL_ITOA_NDIV10MOD(rem, n);
      if (rem) {
        *--buf = digit_to_char(rem);
        // some non-0 trailing digits; now output all digits.
        while (n) {
          GRAEHL_ITOA_NDIV10MOD(rem, n);
          *--buf = digit_to_char(rem);
        }
        return buf;
      }
      ++n_skipped;
    }
    assert(0);
    return 0;
  }
}

// desired feature: itoa(unsigned) = utoa(unsigned)
// positive sign: 0 -> +0, 1-> +1. obviously -n -> -n
template <class Int>
// typename signed_for_int<Int>::original_t instead of Int to give more informative wrong-type message?
char* itoa(char* buf, Int i, bool positive_sign = false) {
  typename signed_for_int<Int>::unsigned_t n = i;
#ifdef __clang__
#include <graehl/shared/warning_push.h>
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4146)
#endif

  if (i < 0) n = -n;
  /* above n=-n is because:

     -(int)-2147483648 = (int) -2147483648-WRONG
     -((unsigned)-2147483648) = 2147483648-RIGHT

     and for all other -n representable
     -(int)-n = (int) n - OK
     -((unsigned)-n) = n - STILL OK

  */
  char* ret = utoa(buf, n);
  if (i < 0) {
    *--ret = '-';
  } else if (positive_sign)
    *--ret = '+';
  return ret;
}

template <class Int>
char* itoa_left_pad(char* buf, char* bufend, Int i, bool positive_sign = false, char pad = '0') {
  typename signed_for_int<Int>::unsigned_t n = i;
  if (i < 0) {
    n = -n;  // see comment above for itoa
#ifdef __clang__
#include <graehl/shared/warning_pop.h>
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    *buf = '-';
  } else if (positive_sign)
    *buf = '+';
  char* r = utoa(bufend, n);
  assert(buf < r);
  left_pad(buf + 1, r, pad);
  return buf;
}

template <class Int>
inline std::string itos(Int n) {
  char buf[signed_for_int<Int>::toa_bufsize];
  char* e = buf + signed_for_int<Int>::toa_bufsize;
  char* p = itoa(e, n);
  return std::string(p, e);
}

template <class Int>
inline std::string utos(Int n) {
  char buf[signed_for_int<Int>::toa_bufsize];
  char* e = buf + signed_for_int<Int>::toa_bufsize;
  char* p = utoa(e, n);
  return std::string(p, e);
}

template <class String, class Int>
inline void utos_append(String& str, Int n) {
  char buf[signed_for_int<Int>::toa_bufsize];
  char* e = buf + signed_for_int<Int>::toa_bufsize;
  char* p = utoa(e, n);
  append(str, p, e);
}

// returns position of '\0' terminating number written starting at to
template <class Int>
inline char* append_utoa(char* to, typename signed_for_int<Int>::unsigned_t n) {
  char buf[signed_for_int<Int>::toa_bufsize];
  char* e = buf + signed_for_int<Int>::toa_bufsize;
  char* p = itoa(e, n);
  int ns = e - p;
  std::memcpy(to, p, ns);
  to += ns;
  *to = 0;
  return to;
}

// returns position of '\0' terminating number written starting at to
template <class Int>
inline char* append_itoa(char* to, typename signed_for_int<Int>::signed_t n) {
  char buf[signed_for_int<Int>::toa_bufsize];
  char* e = buf + signed_for_int<Int>::toa_bufsize;
  char* p = utoa(e, n);
  int ns = e - p;
  std::memcpy(to, p, ns);
  to += ns;
  *to = 0;
  return to;
}


}

#endif
