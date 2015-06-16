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

    named bits set to/from string:

    struct BitsList {
      template <class C>
      static void bits(C &c) {
       c("NAME0"); // starts at value 1
       c("NAME1", 1);
       c("NAME2", 1<<1);
       c("NAME3"); // gets last value (1<<1) * 2
       c("deprecated-synonym-for-NAME4", 4); // parses to value as NAME3 but will never be printed as name
      }
    };

    typedef named_bits<BitsList, unsigned> Bits;

    names that come earlier are preferred for output. values may have more than 1 set bit
*/

#ifndef GRAEHL_SHARED__CONFIGURE_BITS
#define GRAEHL_SHARED__CONFIGURE_BITS
#pragma once

#include <graehl/shared/hex_int.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/split.hpp>

namespace graehl {

static char const* const bit_names_delim = "|,";  // | is nice but hard to cmdline-arg-quote; , is easier

template <class Int = unsigned>
struct bit_names {
  typedef std::pair<std::string, Int> NameValue;
  typedef std::vector<NameValue> NameValues;
  NameValues nv_;
  bool overlapping_;

  void overlapping() {
    overlapping_ = true;
  }

  void operator()(std::string const& str, Int val) {
    nv_.push_back(NameValue(str, val));
    if (!overlapping_ && (known_ & val))
      throw std::runtime_error("bit_names overlapping bits "+to_string_impl(known_&val)+" without first calling overlapping())");
    known_ |= val;
    next_ = val * 2;
  }

  /// returns unknown bits that were set
  Int clear_unknown(Int& val) const {
    Int unk = val & ~known_;
    val &= known_;
    return unk;
  }

  void operator()(std::string const& str) { (*this)(str, next_); }

  bit_names() : next_(1), overlapping_() {}

  std::string usage(bool values = false) const {
    string_builder b;
    usage(b, values);
    return b.str();
  }

  std::string name(Int val) const {
    string_builder b;
    append(val, b);
    return std::string(b.begin(), b.end());
  }

  void usage(string_builder& b, bool values = false) const {
    bool first = true;
    for (typename NameValues::const_iterator i = nv_.begin(), e = nv_.end(); i != e; ++i) {
      b.space_except_first(first, '|');
      b(i->first);
      if (values) b('=')(hex(i->second));
    }
  }

  void append(Int val, string_builder& b) const {
    bool first = true;
    for (typename NameValues::const_iterator i = nv_.begin(), e = nv_.end(); i != e; ++i) {
      Int const mask = i->second;
      if ((val & mask) == mask) {  // did you know: == precedes over &, so we need parens
        b.space_except_first(first, '|');
        b(i->first);
        val &= ~mask;
      }
    }
    if (val) {
      b.space_except_first(first, '|');
      b(to_string(hex_int<Int>(val)));
    }
  }

  std::string const& bitname(Int val, std::string const& fallback) const {
    for (typename NameValues::const_iterator i = nv_.begin(), e = nv_.end(); i != e; ++i)
      if (i->second == val) return i->first;
    return to_string(hex_int<Int>(val));
  }

  Int operator[](std::string const& name) const {
    for (typename NameValues::const_iterator i = nv_.begin(), e = nv_.end(); i != e; ++i)
      if (i->first == name) return i->second;
    return hex_int<Int>(name);
  }

  Int known_;
 private:
  Int next_;
};

template <class NameList, class Int = unsigned>
struct cached_bit_names : bit_names<Int> {
  cached_bit_names() { NameList::bits(*this); }
};

template <class NameList, class Int>
struct parse_bit_names {
  static bit_names<Int> const& names() {
    static cached_bit_names<NameList, Int> gNames;
    return gNames;
  }

  bool operator()(std::string const& s) const {
    val_ |= names()[s];
    return true;
  }

  parse_bit_names(Int& val) : val_(val) {}

  Int& val_;
};

template <class NameList, class Int = unsigned>
struct named_bits : hex_int<Int> {
  typedef void leaf_configure;
  typedef hex_int<Int> Base;
  named_bits() {}

  named_bits(Int i) : Base(i) {}

  Base& base() { return static_cast<Base&>(*this); }

  typedef parse_bit_names<NameList, Int> Names;

  std::string type_string_impl(named_bits const&) {
    return Names::names().usage() + " or 0xfaceb00c hex or decimal";
  }

  static std::string usage(bool values = false) { return Names::names().usage(values); }

  static Int getSingle(std::string const& name) { return Names::names()[name]; }

  /// returns unknown bits that were set
  Int clear_unknown() { return Names::names().clear_unknown(base()); }

  void append(string_builder& b) const { Names::names().append(*this, b); }

  friend void string_to_impl(std::string const& s, named_bits& n) {
    std::string::size_type d = s.find_first_of(bit_names_delim);
    n = 0;
    Names parser(n);
    if (d == std::string::npos) {
      parser(s);
    } else {
      char delim[2];
      delim[1] = 0;
      delim[0] = s[d];
      split_noquote(s, parser, delim);
    }
  }

  friend std::string to_string_impl(named_bits const& n) {
    string_builder b;
    n.append(b);
    return std::string(b.begin(), b.end());
  }

  friend inline std::ostream& operator<<(std::ostream& out, named_bits const& self) {
    self.print(out);
    return out;
  }

  void print(std::ostream& out) const {
    string_builder b;
    append(b);
    out << b;
  }

  static named_bits allbits;
};

template <class NameList, class Int>
named_bits<NameList, Int> named_bits<NameList, Int>::allbits(parse_bit_names<NameList, Int>::names().known_);

}

#endif
