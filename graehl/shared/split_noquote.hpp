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

    visit string fields (stopping early) of string split on arbitrary any-of-these-chars. optionally throw if
   string not completely consumed
*/

#ifndef SPLIT_NOQUOTE_GRAEHL_2015_10_29_HPP
#define SPLIT_NOQUOTE_GRAEHL_2015_10_29_HPP
#pragma once

#include <string>
#include <cstddef>
#include <graehl/shared/input_error.hpp>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

/// returns number of fields visited by f such that return of f(field) was true
template <class Func>
inline std::size_t split_noquote(
    std::string const& csv,
    Func f,  // this returns false if we want to stop; we return the number of fields (up to N) for which f
    // returned true.
    std::string const& delim = ",",  // can be >1 char (string.find)
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


#ifdef GRAEHL_TEST
char const* split_strs[] = {"", ",a", "", 0};
char const* seps[] = {";", ";;", ",,", " ", "=,", ",=", " >||||||<", 0};
char const* split_chrs[] = {";", ",", "a", ";"};

BOOST_AUTO_TEST_CASE(TEST_split_strs) {
  using namespace std;
  {
    std::string str = ";,a;";
    BOOST_CHECK_EQUAL(split_noquote(str, make_expect_visitor(split_chrs), ""), 4);
    BOOST_CHECK_EQUAL(split_noquote(str, make_expect_visitor(split_strs), ";"), 3);
    for (char const** p = seps; *p; ++p) {
      string s;
      char const* sep = *p;
      bool first = true;
      for (char const** q = split_strs; *q; ++q) {
        if (first)
          first = false;
        else
          s.append(sep);
        s.append(*q);
      }
      BOOST_CHECK_EQUAL(split_noquote(s, make_expect_visitor(split_strs), sep), 3);
    }
  }
}
#endif


}

#endif
