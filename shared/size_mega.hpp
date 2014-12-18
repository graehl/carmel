/** \file

    configure size/time in SI giga mega or power-of-2 Mibi Gibi etc..
*/

#ifndef GRAEHL__SHARED__SIZE_MEGA_HPP
#define GRAEHL__SHARED__SIZE_MEGA_HPP
#pragma once

#include <iomanip>
#include <stdexcept>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/print_width.hpp>
#include <graehl/shared/print_read.hpp>
#include <graehl/shared/program_options.hpp>
#include <sstream>
#include <cstddef>
#include <string>
#include <cstdio>

namespace graehl {

/// maxWidth, if positive, limits total number of characters. decimalThousand selects the 10^(3k) SI suffixes
/// (k m g t) instead of 2^(10k) (K G M T)
template <class size_type, class outputstream>
inline outputstream& print_size(outputstream& o, size_type size, bool decimal_thousand = true,
                                int max_width = -1) {
  typedef double size_compute_type;
  size_compute_type thousand = decimal_thousand ? 1000 : 1024;
  if (size < thousand) return o << size;
  size_compute_type base = thousand;
  const char* suffixes = decimal_thousand ? "kmgt" : "KMGT";
  const char* suff = suffixes;
  for (;;) {
    size_compute_type nextbase = base * thousand;
    if (size < nextbase || suff[1] == 0) {
      double d = size / (double)base;
      print_max_width_small(o, d, max_width);
      return o << *suff;
    }

    base = nextbase;
    ++suff;
  }
  return o;  // unreachable
}

template <class size_type>
size_type scale_mega(char suffix, size_type number = 1) {
  switch (suffix) {
    case 't':
      number *= (1000. * 1000. * 1000. * 1000.);
      break;
    case 'T':
      number *= (1024. * 1024. * 1024. * 1024.);
      break;
    case 'g':
      number *= (1000. * 1000 * 1000);
      break;
    case 'G':
      number *= (1024. * 1024 * 1024);
      break;
    case 'm':
      number *= (1000 * 1000);
      break;
    case 'M':
      number *= (1024 * 1024);
      break;
    case 'k':
      number *= 1000;
      break;
    case 'K':
      number *= 1024;
      break;
    default:
      throw std::runtime_error("unknown suffix - expected tTgGmMkK (tera giga mega kilo): "
                               + std::string(1, suffix));
  }
  return number;
}

template <class size_type, class inputstream>
inline size_type parse_size(inputstream& i) {
  double number;
  if (!(i >> number)) goto fail;
  char c;
  if (i.get(c)) return (size_type)scale_mega(c, number);
  if (number - (size_type)number > 100) {
    char buf[100];
    int len = std::sprintf(buf, "Overflow - size too big to fit: %g", number);
    throw std::runtime_error(std::string(buf, len));
  }
  return (size_type)number;
fail:
  throw std::runtime_error(std::string(
      "Expected nonnegative number followed by optional k, m, g, or t (10^3,10^6,10^9,10^12) suffix, or K, "
      "M, G, or T (2^10,2^20,2^30,2^40), e.g. 1.5G"));
}

template <class size_type>
inline size_type size_from_str(std::string const& str) {
  std::istringstream in(str);
  size_type ret = parse_size<size_type, std::istream>(in);
  must_complete_read(in, "Read a size_mega, but didn't parse whole string ");
  return ret;
}

template <class size_type>
inline void size_from_str(std::string const& str, size_type& sz) {
  sz = size_from_str<size_type>(str);
}


template <bool decimal_thousand = true, class size_type = double>
struct size_mega {
  typedef size_mega<decimal_thousand, size_type> self_type;
  size_type size;
  operator size_type&() { return size; }
  operator size_type() const { return size; }
  size_mega() : size() {}
  size_mega(self_type const& o) : size(o.size) {}
  size_mega(size_type size_) : size(size_) {}
  size_mega(std::string const& str, bool unused) { init(str); }
  void init(std::string const& str) { size = (size_type)size_from_str<size_type>(str); }

  self_type& operator=(std::string const& s) { init(s); }

  template <class Ostream>
  void print(Ostream& o) const {
    print_size(o, size, decimal_thousand, 5);
  }
  TO_OSTREAM_PRINT
  FROM_ISTREAM_READ
  template <class I>
  void read(I& i) {
    size = parse_size<size_type>(i);
  }

  typedef void leaf_configure;
};

template <bool Dec, class Sz>
inline char const* type_string(size_mega<Dec, Sz> const&) {
  return "[float]([KMGT]|[kmgt])? (1024^i or 1000^i respectively) e.g. .5K = 512";
}

template <bool Dec, class Sz>
inline void string_to_impl(std::string const& str, size_mega<Dec, Sz>& x) {
  x.init(str);
}

template <bool Dec, class Sz>
inline void validate(size_mega<Dec, Sz>&) {
}


typedef size_mega<false, double> size_bytes;
typedef size_mega<false, unsigned long long> size_bytes_integral;
typedef size_mega<false, std::size_t> size_t_bytes;
typedef size_mega<true, std::size_t> size_t_metric;
typedef size_mega<true, double> size_metric;


}  // graehl

namespace boost {
namespace program_options {
inline void validate(boost::any& v, const std::vector<std::string>& values, size_t* target_type, int) {
  typedef size_t value_type;
  using namespace graehl;

  v = boost::any(graehl::size_from_str<value_type>(get_single_arg(v, values)));
}


}}

#endif
