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

    SipHash 2.4 - cryptographic hash functino w/ decent performance. Intended for
    resistance to intentional collisions by an adversary supplying keys to be
    hashed.

    CityHash and MurmurHash are easily broken (as is Python's hash). Perl uses SipHash.

    "SipHash: a fast short-input PRF" (accepted for presentation at the DIAC workshop and at INDOCRYPT 2012)

    Jean-Philippe Aumasson (Kudelski Security, Switzerland)
    Daniel J. Bernstein (University of Illinois at Chicago, USA)

*/

#ifndef SIPHASH_JG_2013_06_01_HPP
#define SIPHASH_JG_2013_06_01_HPP


#include <boost/cstdint.hpp>
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace siphash {

typedef boost::uint64_t u64;
typedef boost::uint32_t u32;
typedef boost::uint8_t u8;

inline u64 rotateRight(u64 h, int shift) {
  assert(shift && shift<64 && shift>-64);
  return (h >> shift) | (h << (64 - shift));
}

inline u64 rotateLeft(u64 h, int shift) {
  assert(shift && shift<64 && shift>-64);
  return (h << shift) | (h >> (64 - shift));
}

/**
   this will provided incompatible authenticators to the standard siphash 2.4 if your cpu isn't little-endian
*/
struct sipmix {
  u64 v0, v1, v2, v3;
  /**
     k0, k1 (little endian) are the 128 bits of a siphash 2.4 key.
  */
  sipmix(u64 k0, u64 k1)
  {
    clear(k0, k1);
  }

  void clear() {
    // siphash 2.4 authors say these are just randomly generated
    v0 = 0x736f6d6570736575ULL;
    v1 = 0x646f72616e646f6dULL;
    v2 = 0x6c7967656e657261ULL;
    v3 = 0x7465646279746573ULL;
  }

  void clear(u64 k0, u64 k1) {
    clear();
    v0 ^= k0;
    v1 ^= k1;
    v2 ^= k0;
    v3 ^= k1;
  }

  /**
     usage: modify some vN before and after 2 or more repetitions of this:

     'round' as in cryptography - add more rounds to prevent some attacks
  */
  void round() {
    v0 += v1; v1 = rotateLeft(v1, 13); v1 ^= v0; v0 = rotateLeft(v0, 32);
    v2 += v3; v3 = rotateLeft(v3, 16); v3 ^= v2;
    v0 += v3; v3 = rotateLeft(v3, 21); v3 ^= v0;
    v2 += v1; v1 = rotateLeft(v1, 17); v1 ^= v2; v2 = rotateLeft(v2, 32);
  }

  void mix() {
    round(); round();
  }

  /**
     mix a full message block
  */
  void mix(u64 data) {
    v3 ^= data;
    mix();
    v0 ^= data;
  }

  /**
     mix the trailing bytes and return value
  */
  u64 finish(void *remnant, unsigned nremain, u64 nbytes) {
    assert(nremain < 8);
    u64 padded = nbytes << 56;
    if (nremain)
      std::memcpy((void*)&padded, remnant, nremain);
    mix(padded);
    return finish();
  }

  u64 finish() {
    v2 ^= 0xff; // i wonder what this is for!
    round(); round(); round(); round();
    return get();
  }

  u64 finish(u64 nbytes) {
    mix(nbytes << 56);
    return finish();
  }

  /**
     returns the (after conversion to little endian if necessary) bytes of siphash 2.4 authenticator.
  */
  u64 get() const {
    return v0 ^ v1 ^ v2 ^ v3;
  }
};

/**
   return (little-endian) siphash 2.4 of little-endian 128-bit key in
   little-endian parts [k0,k1], each of which is little-endian. data is
   n64*8 bytes long
*/
inline u64 siphash(u64 *data, unsigned n64, u64 k0 = 0, u64 k1 = 0) {
  sipmix s(k0, k1);
  for (unsigned i = 0; i < n64; ++i)
    s.mix(data[i]);
  return s.finish((u64)n64 * 8);
}

/**
   msg should be 8-byte aligned or you may have bad performance. returns authenticator for msg given key [k0, k1]
*/
inline u64 siphash(void *msg, unsigned len, u64 k0 = 0, u64 k1 = 0) {
  sipmix s(k0, k1);
  unsigned const remain = len & 7;
  u64 *end = (u64*)((char*)msg + len - remain);
  for (u64 *block = (u64*)msg; block < end; ++block)
    s.mix(*block);
  return s.finish(end, remain, len);
}

/**
   i wonder if this can fully inline, including not really constructing a sipmix object?
*/
inline u64 siphashint(u64 x) {
  return siphash(&x, 1);
}

inline u64 siphashpointer(void *p) {
  u64 x = (u64)p;
  return siphash(&x, 1);
}

#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE(siphash_basic) {
  u64 k0 = 0x0706050403020100ULL;
  u64 k1 = 0x0F0E0D0C0B0A0908ULL;
  BOOST_CHECK_EQUAL(siphash(0, 0, k0, k1),   0x726fdb47dd0e0e31);
  BOOST_CHECK_EQUAL(siphash(&k0, 1, k0, k1), 0xab0200f58b01d137);
}
#endif

}

#endif // SIPHASH_JG_2013_06_01_HPP
