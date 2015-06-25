/** \file \author Jonathan Graehl <graehl@gmail.com>

    replace 0-9 ascii chars with another ascii replacement

    To the extent possible under law, the author(s) have dedicated all copyright
    and related and neighboring rights to this software to the public domain
    worldwide. This software is distributed without any warranty.
*/

#ifndef REPLACEDIGITS_GRAEHL_2015_06_25_H
#define REPLACEDIGITS_GRAEHL_2015_06_25_H
#pragma once

#include <string>
#include <utility>

namespace graehl {

inline bool ascii_digit(char c) {
  return c >= '0' && c <= '9';
}

struct replace_digits {
  char map_digits;
  replace_digits(char map_digits = '@') : map_digits(map_digits) {}

  /// \return whether anything was replaced
  bool replaced(char* i, char* end) const {
    for (; i != end; ++i)
      if (ascii_digit(*i)) {
        *i = map_digits;
        while (++i != end)
          if (ascii_digit(*i)) *i = map_digits;
        return true;
      }
    return false;
  }
  /// maybe: only if non-0 map_digits, do the thing
  bool maybe_replaced(char* i, char* end) const { return map_digits && replaced(i, end); }

  void replace(char* i, char* end) const {
    for (; i != end; ++i)
      if (ascii_digit(*i)) *i = map_digits;
  }
  void maybe_replace(char* i, char* end) const {
    if (map_digits) replace(i, end);
  }

  void replace(std::string& str, std::string::size_type i = 0) const {
    std::string::size_type n = str.size();
    char* d = (char *)str.data(); // although only C++11 officially allows this, in reality everyone does
    replace(d + i, d + n);
  }
  void maybe_replace(std::string& str, std::string::size_type i = 0) const {
    if (map_digits) replace(str, i);
  }
};


}

#endif
