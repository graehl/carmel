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
