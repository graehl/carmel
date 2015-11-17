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

 .
*/

#ifndef STRIDE_JG_2014_12_09_HPP
#define STRIDE_JG_2014_12_09_HPP

#include <iterator>
#include <cstring>

namespace graehl {

template <class Iter>
struct StrideIterator {
  static unsigned stride;
  typedef std::size_t difference_type;
  typedef std::iterator_traits<Iter> Traits;
  typedef typename Traits::value_type Value;
  struct ValueBytes {
    Value* bytes;
    operator Value&() const { return *bytes; }
    ValueBytes() : bytes((Value*)malloc(stride)) {}
    ~ValueBytes() { free(bytes); }
    ValueBytes(Value const& o) : bytes((Value*)malloc(stride)) { operator=(o); }
    void operator=(Value const& o) { std::memcpy(bytes, &o, stride); }
  };
  typedef ValueBytes value_type;
  typedef Value& reference;
  typedef typename Traits::pointer pointer;
  typedef typename Traits::iterator_category iterator_category;

  Iter i;
  StrideIterator& operator++() {
    i += stride;
    return *this;
  }
  StrideIterator& operator--() {
    i -= stride;
    return *this;
  }
  template <class Offset>
  StrideIterator& operator+=(size_t x) {
    i += x * stride;
    return *this;
  }
  template <class Offset>
  StrideIterator& operator-=(Offset x) {
    i -= x * stride;
    return *this;
  }
  template <class Offset>
  StrideIterator operator+(Offset x) const {
    StrideIterator r(*this);
    r.i += x * stride;
    return r;
  }
  template <class Offset>
  StrideIterator operator-(Offset x) const {
    StrideIterator r(*this);
    r.i -= x * stride;
    return r;
  }
  difference_type operator-(StrideIterator const& o) const { return (i - o.i) / stride; }
  StrideIterator operator++(int) {
    StrideIterator r = *this;
    i += stride;
    return r;
  }
  StrideIterator operator--(int) {
    StrideIterator r = *this;
    i -= stride;
    return r;
  }
  reference operator*() const { return *i; }
  bool operator==(StrideIterator const& o) const { return i == o.i; }
  bool operator!=(StrideIterator const& o) const { return i != o.i; }
  bool operator<(StrideIterator const& o) const { return i < o.i; }
  bool operator<=(StrideIterator const& o) const { return i <= o.i; }
  bool operator>(StrideIterator const& o) const { return i > o.i; }
  bool operator>=(StrideIterator const& o) const { return i >= o.i; }
};

template <class Iter>
unsigned StrideIterator<Iter>::stride;


}

#endif
