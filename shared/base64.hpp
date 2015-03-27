/** \file

 encode bytes or integers as ascii A-Za-z0-9-_ (base64url)

 see https://en.wikipedia.org/wiki/Base64#Variants_summary_table
*/

#ifndef BASE64_JG_2015_03_25_HPP
#define BASE64_JG_2015_03_25_HPP
#pragma once

#include <cassert>
#include <cstdlib>

namespace graehl {

static char const *base64url = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

/// \return: each [0..63] has a unique non-0 byte. fairly slow.
inline bool good_base64_code(char const* base64code) {
  assert(sizeof(char) == 1);
  char seen[256]; //TODO: bitvector instead?
  std::memset(seen, 0, 256);
  for (unsigned i = 0; i < 64; ++i) {
    unsigned char c = base64code[i];
    assert((unsigned)c < 256);
    if (!c) return false;
    if (seen[c]) return false;
    seen[c] = 1;
  }
  return true;
}

/// \return ceil(8*n/6) since 2^6 = 64 - 6 bits of info per char
template <class Int>
inline Int base64_chars_for_bytes(Int n) {
  return (n * 4 + 2) / 3;
}

/// OutAscii output iterator gets var-length Little Endian (lsb first) encoding
template <class OutAscii, class Int>
OutAscii base64LE(OutAscii o, Int x, char const* base64code = base64url) {
  assert(good_base64_code(base64code));
  while(x) {
    *o = base64code[x & 63];
    x >>= 6;
    ++o;
  }
}

/// OutAscii output iterator gets fixed-length Little Endian (lsb first) encoding. not using '=' char for padding but rather 0 bits.
template <class OutAscii, class Int>
OutAscii base64LE_pad(OutAscii o, Int x, char const* base64code = base64url) {
  assert(good_base64_code(base64code));
  unsigned i = 0, N = (sizeof(Int) * 4 + 2) / 3;
  for (; i != N; ++i) {
    *o = base64code[x & 63];
    x >>= 6;
    ++o;
  }
}


template <class String, class Int>
void base64LE_append(String &s, Int x, char const* base64code = base64url) {
  assert(good_base64_code(base64code));
  while(x) {
    s.push_back(base64code[x & 63]);
    x >>= 6;
  }
}

template <class String, class Int>
void base64LE_append_pad(String &s, Int x, char const* base64code = base64url) {
  assert(good_base64_code(base64code));
  unsigned i = s.size(), N = i + (sizeof(Int) * 4 + 2) / 3;
  s.resize(N);
  for(; i != N; ++i) {
    s[i] = base64code[x & 63];
    x >>= 6;
  }
}

}

#endif
