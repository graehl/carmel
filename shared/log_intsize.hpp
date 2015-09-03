/** \file

  compile time constnats for packed bit representations w/ >1-byte int
  blocksize, we need to divide/remainder a bit index by the blocksize. this can
  be done with a log2(#bits-in-block) bit shift on the index.
*/

#ifndef GRAEHL__LOG_INTSIZE__HPP
#define GRAEHL__LOG_INTSIZE__HPP
#pragma once


namespace graehl {

template <unsigned n>
struct log_n
{};
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
struct log_intsize
{
  enum { value = log_n<sizeof(Int)>::value };
};

}

#endif
