/** \file

    visit string fields (stopping early) of string split on arbitrary any-of-these-chars. optionally throw if string not completely consumed
*/

#ifndef SPLIT_NOQUOTE_GRAEHL_2015_10_29_HPP
#define SPLIT_NOQUOTE_GRAEHL_2015_10_29_HPP
#pragma once

#include <string>
#include <cstddef>
#include <graehl/shared/input_error.hpp>

namespace graehl {

/// returns number of fields visited by f such that return of f(field) was true
template <class Func>
inline std::size_t split_noquote(
    std::string const& csv,
    Func f,  // this returns false if we want to stop; we return the number of fields (up to N) for which f
    // returned true.
    std::string const& delim = ",", // string find allows alternative chars here
    std::size_t N = (std::size_t)-1,  // max number of calls to f (even if more fields exist)
    bool leave_tail = true,  // if N reached and there's more string left, include it in final call to f
    bool must_complete = false  // throw if whole string isn't consumed (meaningless unless leave_tail==false)
    ) {
  using namespace std;
  string::size_type delim_len = delim.length();
  if (delim_len == 0) {  // split by characters as special case
    string::size_type i = 0, n = csv.size();
    for (; i < n; ++i)
      if (!f(string(csv, i, 1))) return i;
    return i;
  }
  string::size_type pos = 0, nextpos;
  std::size_t n = 0;
  while (n < N && (!leave_tail || n + 1 < N) && (nextpos = csv.find(delim, pos)) != string::npos) {
    if (!f(string(csv, pos, nextpos - pos))) return n;
    ++n;
    pos = nextpos + delim_len;
  }
  if (csv.length() != 0) {
    if (must_complete && n + 1 != N)
      throw_parse_error(csv, "Expected exactly " + to_string(N) + " " + delim + " separated fields", pos);
    if (!f(string(csv, pos, csv.length() - pos))) return n;
    ++n;
  }
  return n;
}


}

#endif
