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

  compile time constants for packed bit representations w/ >1-byte int
  blocksize, we need to divide/remainder a bit index by the blocksize. this can
  be done with a log2(#bits-in-block) bit shift on the index.
*/

#ifndef GRAEHL__LOG_INTSIZE__HPP
#define GRAEHL__LOG_INTSIZE__HPP
#pragma once


namespace graehl {

template <unsigned n>
struct log_n {};
template <>
struct log_n<1> {
  enum { value = 0 };
};
template <>
struct log_n<2> {
  enum { value = 1 };
};
template <>
struct log_n<4> {
  enum { value = 2 };
};
template <>
struct log_n<8> {
  enum { value = 3 };
};
template <>
struct log_n<16> {
  enum { value = 4 };
};
template <class Int>
struct log_intsize {
  enum { value = log_n<sizeof(Int)>::value };
};


}

#endif
