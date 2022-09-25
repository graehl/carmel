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

 encode bytes or integers as ascii A-Za-z0-9-_ (base64url)

 see https://en.wikipedia.org/wiki/Base64#Variants_summary_table
*/

#ifndef BASE64_JG_2015_03_25_HPP
#define BASE64_JG_2015_03_25_HPP
#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>

namespace graehl {

static constexpr char const* base64url = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

inline bool isBase64UrlChar(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '0') || c == '-' || c == '_';
}

inline int base64UrlValue(char c) {
  if (c == '-')
    return 62;
  if (c == '_')
    return 63;
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return 26 + c - 'a';
  if (c >= '0' && c <= '9')
    return 52 + c - '0';
  return -1;
}

inline bool isSingleCharBase64Url(unsigned n) {
  return n < 64;
}

inline char singleCharBase64Url(unsigned n) {
  return n < 64 ? base64url[n] : 0;
}

/// \return: each [0..63] has a unique non-0 byte. fairly slow.
inline bool good_base64_code(char const* base64code) {
  assert(sizeof(char) == 1); // NOLINT
  if (!base64code)
    return false;
  if (std::strlen(base64code) != 64)
    return false;
  char seen[128]; // TODO: bitvector instead?
  std::memset((void*)seen, 0, 128);
  for (unsigned i = 0; i < 64; ++i) {
    unsigned char c = base64code[i];
    assert((unsigned)c < 256);
    if (!c)
      return false;
    if (seen[c])
      return false;
    seen[c] = 1;
  }
  return true;
}

#if GRAEHL_TEST
BOOST_AUTO_TEST_CASE(TestBase64) {
  BOOST_REQUIRE(good_base64_code(base64url));
  BOOST_CHECK_EQUAL(base64UrlValue('9'), 61);
  BOOST_CHECK_EQUAL(base64UrlValue('-'), 62);
  BOOST_CHECK_EQUAL(base64UrlValue('_'), 63);
  BOOST_CHECK_EQUAL(base64UrlValue('B'), 1);
  BOOST_CHECK_EQUAL(base64UrlValue('b'), 27);
  char seen[128] = {};
  for (unsigned i = 0; i < 64; ++i) {
    unsigned char c = base64url[i];
    seen[c] = 1;
    BOOST_CHECK_EQUAL(base64UrlValue(c), i);
  }
  unsigned nseen = 0;
  for (unsigned i = 0; i < 128; ++i)
    if (seen[i])
      ++nseen;
    else
      BOOST_CHECK_EQUAL(base64UrlValue((char)i), -1);
  BOOST_CHECK_EQUAL(nseen, 64);
}
#endif

/// \return ceil(8*n/6) since 2^6 = 64-6 bits of info per char
template <class Int>
inline Int base64_chars_for_bytes(Int n) {
  return (n * 4 + 2) / 3;
}

/// OutAscii output iterator gets var-length Little Endian (lsb first) encoding
template <class OutAscii, class Int>
OutAscii base64LE(OutAscii o, Int x) {
  assert(good_base64_code(base64url));
  while (x) {
    *o = base64url[x & 63];
    x >>= 6;
    ++o;
  }
}

/// OutAscii output iterator gets fixed-length Little Endian (lsb first) encoding. not using '=' char for
/// padding but rather 0 bits.
template <class OutAscii, class Int>
OutAscii base64LE_pad(OutAscii o, Int x) {
  assert(good_base64_code(base64url));
  unsigned i = 0, N = (sizeof(Int) * 4 + 2) / 3;
  for (; i != N; ++i) {
    *o = base64url[x & 63];
    x >>= 6;
    ++o;
  }
}


template <class String, class Int>
void base64LE_append(String& s, Int x) {
  assert(good_base64_code(base64url));
  if (!x)
    s.push_back(base64url[0]);
  else
    while (x) {
      s.push_back(base64url[x & 63]);
      x >>= 6;
    }
}

template <class String, class Int>
void base64LE_append_pad(String& s, Int x) {
  assert(good_base64_code(base64url));
  unsigned i = s.size(), N = i + (sizeof(Int) * 4 + 2) / 3;
  s.resize(N);
  for (; i != N; ++i) {
    s[i] = base64url[x & 63];
    x >>= 6;
  }
}


} // namespace graehl

#endif
