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
#ifndef FIXEDBUFFER_H
#define FIXEDBUFFER_H

#include <stdexcept>
#include <cstddef>

#ifdef DEBUG
# include <cstring>
# endif

namespace graehl {

template <class T, bool PlainData = false>
// no bounds checking or growing ...
struct fixed_buffer {
  T *begin_;
  T *end_;
  fixed_buffer(size_t sz) : begin_((T*)::operator new(sizeof(T)*sz)), end_(begin_) {
    //        INFOL(99,"FixedBuffer","New buffer of " << sz << " elements sized " << sizeof(T) << " bytes.");
# if ASSERT_LVL > 100
    std::memset(begin_, 0x77, sizeof(T)*sz);
#  endif
  }
  void resize(size_t sz)
  {
    throw std::runtime_error("tried to resize a fixed-sized buffer (shoulda made it bigger to start with)!");
  }
  operator T *() {
    return begin_;
  }
  operator const T *() const {
    return begin_;
  }
  template <class T2>
  inline void push_back(const T2& t) {
    if (PlainData)
      new(end_++) T(t);
    else
      *end_++=t;
  }
  inline void push_back() {
    if (PlainData)
      new(end_++) T();
    else
      ++end_;
  }
  inline T *push_back_raw() {
    return end_++;
  }
  typedef T* iterator;
  typedef const T* const_iterator;
  iterator begin() {return begin_; }
  const_iterator begin() const {return begin_; }
  iterator end() {return end_; }
  const_iterator end() const {return end_; }
  bool empty() const {
    return !size();
  }
  ptrdiff_t size() const {
    return end_-begin_;
  }
  ~fixed_buffer() {
    ::operator delete(begin_);
  }
  void clear() {
    if (PlainData)
      end_ = begin();
    else {
      while (--end_ >= begin())
        end_->~T();
      ++end_; // so we're idempotent
    }
  }
  T &at(size_t index) {
    if (index > size())
      throw std::out_of_range();
    return begin()[index];
  }
  const T &at(size_t index) const {
    if (index > size())
      throw std::out_of_range();
    return begin()[index];
  }
};

template <class T, size_t sz, bool PlainData = false>
// no bounds checking or growing ...
struct fixed_buffer_c {
  char begin_[sz*sizeof(T)];
  T *end_;
  fixed_buffer_c() : end_((T*)begin_) {
# if ASSERT_LVL > 100
    std::memset(begin_, 0x77, sizeof(T)*sz);
# endif
  }
  void resize(size_t /*size*/)
  {
    throw std::runtime_error("tried to resize a fixed-sized buffer (shoulda made it bigger to start with)!");
  }
  operator T *() {
    return (T*)begin_;
  }
  operator const T *() const {
    return (const T*)begin_;
  }
  template <class T2>
  inline void push_back(const T2& t) {
# if ASSERT_LVL > 50
    assert(size()<sz);
#  endif
    if (PlainData)
      new(end_++) T(t);
    else
      *end_++=t;
  }
  inline void push_back() {
# if ASSERT_LVL > 50
    assert(size()<sz);
#  endif
    if (PlainData)
      new(end_++) T();
    else
      push_back_raw();
  }
  inline T *push_back_raw() {
# if ASSERT_LVL > 50
    assert(size()<sz);
#  endif
    return end_++;
  }
  typedef T* iterator;
  typedef const T* const_iterator;
  iterator begin() {return (iterator)begin_; }
  const_iterator begin() const {return (const_iterator)begin_; }
  iterator end() {return end_; }
  const_iterator end() const {return end_; }
  bool empty() const {
    return !size();
  }
  size_t size() const {
    return end_-begin();
  }
  size_t capacity() const {
    return sz;
  }
  T &at(size_t index) {
    if (index > size())
      throw std::out_of_range();
    return begin()[index];
  }
  const T &at(size_t index) const {
    if (index > size())
      throw std::out_of_range();
    return begin()[index];
  }
  void clear() {
    if (PlainData)
      end_ = begin();
    else {
      while (--end_ >= begin())
        end_->~T();
      ++end_; // so we're idempotent
    }
  }
};

}//graehl

#endif
