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

    a band matrix is a sparse matrix whose non-zero entries are confined to a diagonal band, comprising the
   main diagonal and zero or more diagonals on either side.

    for POD T only.
*/

#ifndef BAND_MATRIX_GRAEHL_2015_07_30_HPP
#define BAND_MATRIX_GRAEHL_2015_07_30_HPP
#pragma once

#include <graehl/shared/cpp11.hpp>
#include <cassert>
#include <cstdlib>

namespace graehl {

template <class T>
struct band_matrix {
  typedef unsigned size_type;
  typedef T value_type;
  typedef T* iterator;
  typedef T* const_iterator;
  bool empty() const { return !data_; }
  size_type size() const { return rows_ * band_; }
  T* begin() const { return (T*)data_; }
  T* end() const { return (T*)(data_ + bytes()); }

  /// x(row,col) == x[row][col]. \return 0 if empty()
  T* operator[](size_type row) const { return (T*)(data_ + (stride_ * row)); }
  /// x(row,col) == x[row][col]
  T& operator()(size_type row, size_type col) const {
    assert(row < rows_);
    assert(data_);
    assert(col >= row);
    assert(col - row < band_);
    return operator[](row)[col];
  }
  T operator()(size_type row, size_type col, T const& missing) const {
    return col >= row && col - row < band_ ? operator[](row)[col] : missing;
  }


  char* data_;  // data_ + (stride_ * row) + col
  size_type stride_;
  size_type rows_;
  size_type band_;
  std::size_t bytes() const { return size() * sizeof(T); }
  band_matrix() { init(); }
  /// contents are 0-bytes
  band_matrix(size_type rows, size_type band) {
    if (!rows || !band)
      init();
    else
      init(rows, band);
  }
  /// contents initialized to 'fill'
  band_matrix(size_type rows, size_type band, T const& fill) { init(rows, band, fill); }
  ~band_matrix() { free(); }

#if GRAEHL_CPP11
  /// move
  band_matrix(band_matrix&& o) noexcept {
    std::memcpy(this, &o, sizeof(band_matrix));  // NOLINT
    o.clear();
  }
  /// move
  band_matrix& operator=(band_matrix&& o) noexcept {
    assert(&o != this);  // std::vector doesn't check for self-move so why should we?
    free();
    std::memcpy(this, &o, sizeof(band_matrix));  // NOLINT
    o.clear();
    return *this;
  }
#endif
  band_matrix(band_matrix const& o) { init(o); }
  void operator=(band_matrix const& o) {
    if (&o != this) {
      free();
      init(o);
    }
  }

  /// contents initialized to x
  void init(size_type rows, size_type band, T const& x) {
    init_without_zeroing(rows, band);
    fill(x);
  }

  void init_without_zeroing(size_type rows, size_type band) {
    assert(band);
    stride_ = sizeof(T) * (band - 1);
    rows_ = rows;
    band_ = band;
    data_ = (char*)std::malloc(bytes());
  }

  void init(size_type rows, size_type band) {
    init_without_zeroing(rows, band);
    zero();
  }

  void init(band_matrix const& o) {
    init_without_zeroing(o.rows_, o.band_);
    std::memcpy(data_, o.data_, bytes());
  }

  void zero() { std::memset(data_, 0, bytes()); }

  void fill(T const& x) {
    for (iterator i = begin(), e = end(); i != e; ++i) *i = x;
  }
  void clear() {
    free();
    init();
  }
  void init() {
    data_ = 0;
    rows_ = 0;
    stride_ = 0;
  }
  friend inline std::ostream& operator<<(std::ostream& out, band_matrix const& self) {
    self.print(out);
    return out;
  }
  void print(std::ostream& out, char const* space = " ", char const* nl = "\n", bool rownumber = true) const {
    T* d = begin();
    if (rownumber) out << rows_ << " x " << band_ << " (column index = span size; row: span start)" << nl;
    for (size_type i = 0; i < rows_; ++i) {
      if (rownumber) out << i << ":\t";
      char const* sp = "";
      for (size_type j = 0; j < band_; ++j) {
        out << sp << *d++;
        sp = space;
      }
      out << nl;
    }
  }

 private:
  void free() { std::free(data_); }
};
}

#ifdef GRAEHL_TEST
namespace graehl {

template <class T>
inline void test_band_matrix() {
  typedef band_matrix<T> M;
  T k = 13;
  for (T rows = 1; rows <= 3; ++rows) {
    for (T band = 1; band <= 4; ++band) {
      M m(rows, band, k);
      BOOST_CHECK_EQUAL(m(rows-1, rows - 1), k);
      for (T i = 0; i < rows; ++i)
        for (T j = 0; j < band; ++j) {
          BOOST_CHECK_EQUAL(m[i][i + j], k);
          BOOST_CHECK_EQUAL(m(i, i + j), k);
          m(i, i + j) = i + j;
          BOOST_CHECK_EQUAL(m[i][i + j], i + j);
          BOOST_CHECK_EQUAL(m(i, i + j), i + j);
        }
      BOOST_CHECK_EQUAL(m(rows-1, rows - 1), rows - 1);
      for (T i = 0; i < rows; ++i)
        for (T j = 0; j < band; ++j) BOOST_CHECK_EQUAL(m[i][i + j], i + j);
    }
  }
}

BOOST_AUTO_TEST_CASE(band_matrix_test_case) {
  test_band_matrix<char>();
  test_band_matrix<int>();
  test_band_matrix<double>();
}

#endif


#endif
