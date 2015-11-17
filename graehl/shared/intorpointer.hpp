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
// if you ever wanted to store a discriminated union of pointer/integer without an extra boolean flag, this will do it, assuming your pointers are never odd.
#ifndef INTORPOINTER_HPP
#define INTORPOINTER_HPP

#include <cassert>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

template <class Pointed = void, class Int = std::size_t>
struct IntOrPointer {
  typedef Pointed pointed_type;
  typedef Int integer_type;
  typedef Pointed *value_type;
  typedef IntOrPointer<Pointed, Int> self_type;
  union {
    value_type p; // must be even (guaranteed unless you're pointing at packed chars)
    integer_type i; // stored as 2*data+1, so only has half the range (one less bit) of a normal integer_type
  };
  bool is_integer() const { return i & 1; }
  bool is_pointer() const { return !is_integer(); }
  value_type & pointer() { return p; }
  const value_type & pointer() const { return p; }
  integer_type integer() const { return i >> 1; }
  /// if sizeof(C) is even, could subtract sizeof(C)/2 bytes from base and add directly i * sizeof(C)/2 bytes
  template <class C>
  C* offset_integer(C *base) {
    return base + integer();
  }
  template <class C>
  C* offset_pointer(C *base) {
    //return C + offset_to_index(pointer());
    return offset_ptradd_rescale(base, pointer());
  }
  void operator = (unsigned j) { i = 2*(integer_type)j+1; }
  void operator = (int j) { i = 2*(integer_type)j+1; }
  void set_integer(Int j) { i = 2*j+1; }
  template <class C>
  void operator = (C j) { i = 2*(integer_type)j+1; }
  template <class C>
  void operator = (C *ptr) { p = ptr; }
  void operator = (value_type v) { p = v; }
  IntOrPointer() {}
  IntOrPointer(const self_type &s) : p(s.p) {}
  void operator = (const self_type &s) { p = s.p; }
  template <class C>
  bool operator ==(C* v) const { return p==v; }
  template <class C>
  bool operator ==(const C* v) const { return p==v; }
  template <class C>
  bool operator ==(C j) const { return integer() == j; }
  bool operator ==(self_type s) const { return p==s.p; }
  bool operator !=(self_type s) const { return p!=s.p; }
  operator bool() const {
    return p;
  }
  friend inline std::ostream& operator<<(std::ostream &out, IntOrPointer const& self) {
    self.print(out);
    return out;
  }
  void print(std::ostream &out) const {
    if (is_integer())
      out << integer();
    else {
      out << "0x" << std::hex << (std::size_t)pointer() << std::dec;
    }
  }
};

#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE( TEST_INTORPOINTER )
{
  int i = 3, k;
  IntOrPointer<int> p(5);
  IntOrPointer<int> v(&i);
  BOOST_CHECK(p.is_integer());
  BOOST_CHECK(!p.is_pointer());
  BOOST_CHECK(p == 5);
  p = 0;
  BOOST_CHECK(p.is_integer());
  BOOST_CHECK(p.integer() == 0);
  BOOST_CHECK(p == 0);
  BOOST_CHECK(p!=v);

  p=&i;
  BOOST_CHECK(p.is_pointer());
  BOOST_CHECK(p.pointer() == &i);
  BOOST_CHECK(p == &i);
  BOOST_CHECK(p==v);
}
#endif
}
#endif
