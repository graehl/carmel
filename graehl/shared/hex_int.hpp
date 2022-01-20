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
/*
  \file

  wrapped (default init to 0) integral types that do iostream with hex format (both in and out) - 0x
  prefix is mandatory, else read as regular int

  difference form strtoul: (no octal handling for 0123-who uses that anyway?)

*/

#ifndef GRAEHL__SHARED__HEX_INT_HPP
#define GRAEHL__SHARED__HEX_INT_HPP
#pragma once

#include <graehl/shared/atoi_fast.hpp>
#include <graehl/shared/bit_arithmetic.hpp>
#include <graehl/shared/have_64_bits.hpp>
#include <graehl/shared/print_read.hpp>
#include <graehl/shared/string_buffer.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/type_string.hpp>
#include <iomanip>

namespace graehl {

template <class I>
struct hex_int {
  typedef typename signed_for_int<I>::unsigned_t U;
  I i;
  U count_set_bits() const { return graehl::count_set_bits(i); }
  typedef void leaf_configure;
  void assign(std::string const& s) {
    std::string::size_type const sz = s.size();
    if (sz >= 2 && s[0] == '0' && s[1] == 'x')
      i = (I)hextou<U>(&s[2], &s[sz]);
    else
      string_to(s, i);
  }

  friend inline void string_to_impl(std::string const& s, hex_int& me) { me.assign(s); }
  friend inline std::string type_string_impl(hex_int const& me) {
    return "(hexadecimal) " + type_string(me.i);
  }

  hex_int() : i() {}
  constexpr hex_int(I i) : i(i) {}
  explicit hex_int(std::string const& s) { assign(s); }
  operator I const&() const { return i; }
  operator I&() { return i; }
  void operator=(I to) { i = to; }
  typedef hex_int<I> self_type;
  TO_OSTREAM_PRINT
  FROM_ISTREAM_READ
  template <class S>
  void print(S& s) const {
    s << '0' << 'x' << std::hex << U(i) << std::dec;
  }
  template <class S>
  void read(S& s) {
    char c;
    if (!s.get(c)) return;
    if (c == '0') {
      i = 0;
      if (!s.get(c)) return;
      if (c == 'x') {
        U u;
        if (!(s >> std::hex >> u >> std::dec)) return;
        i = u;
      } else {
        s.unget();  // actual number starting with 0. //octal for consistency with string_to.
        if (std::isdigit(c)) s >> std::oct >> i;
      }
    } else {
      // regular int, maybe signed
      s.unget();
      s >> i;
    }
  }
};

template <class I>
hex_int<I> hex(I i) {
  return hex_int<I>(i);
}

template <class I>
hex_int<std::size_t> hex(I* i) {
  return hex_int<std::size_t>((std::size_t)i);
}

#define DEFINE_HEX_INT(i) typedef hex_int<i> hex_##i;
#define DEFINE_HEX_INTS(i) DEFINE_HEX_INTS(i) DEFINE_HEX_INTS(u##i)
// these are brought into global namespace by string_to-deal, or use boost::intN_t instead
DEFINE_HEX_INT(int8_t);
DEFINE_HEX_INT(int16_t);
DEFINE_HEX_INT(int32_t);
#if HAVE_64_BITS
DEFINE_HEX_INT(int64_t);
#endif

DEFINE_HEX_INT(uint8_t);
DEFINE_HEX_INT(uint16_t);
DEFINE_HEX_INT(uint32_t);
#if HAVE_64_BITS
DEFINE_HEX_INT(uint64_t);
#endif


}

#endif
