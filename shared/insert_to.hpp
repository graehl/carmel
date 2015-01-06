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
#ifndef GRAEHL_SHARED__INSERT_TO_HPP
#define GRAEHL_SHARED__INSERT_TO_HPP
#pragma once

#include <cassert>

namespace graehl {

template <class O>
struct put_iterator {
  O *o;
  explicit put_iterator(O &o) : o(&o) {}
  typedef char value_type;
  typedef put_iterator self_type;
  void operator=(put_iterator const& other) const { assert(o == other.o); }
  template <class V>
  void operator=(V const& v) const { o->put(v); }
  self_type const& operator++(int) const { return *this; }
  self_type const& operator++() const { return *this; }
  self_type const& operator*() const { return *this; }
  bool operator!=(self_type const&) const {return true;}
  bool operator==(self_type const&) const {return false;}
};

template <class O>
put_iterator<O> put_to(O &o) {
  return put_iterator<O>(o);
}

template <class O>
struct insertion_iterator {
  O &o;
  explicit insertion_iterator(O &o) : o(o) {}
  typedef void value_type;
  typedef insertion_iterator self_type;
  template <class V>
  void operator=(V const& v) const { o << v;}
  insertion_iterator const& operator++(int) const { return *this; }
  insertion_iterator const& operator++() const { return *this; }
  insertion_iterator const& operator*() const { return *this; }
  bool operator!=(self_type const&) const {return true;}
  bool operator==(self_type const&) const {return false;}
};

template <class O>
insertion_iterator<O> insert_to(O &o) {
  return insertion_iterator<O>(o);
}

// like std::ostream_iterator but works for more than ostream.
template <class O, class V>
struct insertion_iterator_typed {
  typedef V value_type;
  O &o;
  explicit insertion_iterator_typed(O &o) : o(o) {}
  typedef insertion_iterator_typed self_type;
  void operator=(V const& v) const { o << v;}
  self_type const& operator++(int) const { return *this; }
  self_type const& operator++() const { return *this; }
  self_type const& operator*() const { return *this; }
  bool operator!=(self_type const&) const {return true;}
  bool operator==(self_type const&) const {return false;}
};

template <class V, class O>
insertion_iterator_typed<O, V> insert_to_typed(O &o) {
  return insertion_iterator_typed<O, V>(o);
}


}

#endif
