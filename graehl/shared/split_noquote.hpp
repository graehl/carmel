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

#include <graehl/shared/input_error.hpp>
#include <cstddef>
#include <string>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

/// returns number of fields visited by f such that return of f(field) was true
template <class Func>
inline std::size_t split_noquote(
  std::string const& csv,
  Func f, // this returns false if we want to stop; we return the number of fields (up to N) for which f
  // returned true.
  std::string const& delim = ",", // can be >1 char (string.find)
  std::size_t N = (std::size_t)-1, // max number of calls to f (even if more fields exist)
  bool leave_tail = true, // if N reached and there's more string left, include it in final call to f
  bool must_complete = false // throw if whole string isn't consumed (meaningless unless leave_tail==false)
) {
  using namespace std;
  string::size_type delim_len = delim.size();
  if (delim_len == 0) { // split by characters as special case
    string::size_type i = 0, n = csv.size();
    for (; i < n; ++i)
      if (!f(string(csv, i, 1)))
        return i;
    return i;
  }
  if (leave_tail && N == 1)
    return (bool)f(csv);
  string::size_type pos = 0, nextpos;
  std::size_t n = 0;
  for (auto nsplit = N - leave_tail; n < nsplit && (nextpos = csv.find(delim, pos)) != string::npos;
       ++n, pos = nextpos + delim_len)
    if (!f(string(csv, pos, nextpos - pos)))
      return n;
  if (must_complete && !leave_tail && csv.find(delim, pos) != string::npos)
    throw_parse_error(csv, "Expected exactly " + to_string(N) + " " + delim + " separated fields", pos);
  return n + (bool)f(string(csv, pos));
}


#ifdef GRAEHL_TEST
namespace unit_test {
char const* split_input = ";,a;b||";
char const* split_chrs[] = {";", ",", "a", ";", "b", "|", "|", 0};
char const* split_strs[] = {"", ",a", "b||", 0};
char const* split_seps[] = {";", ";;", ",,", " ", "=,", ",=", " >||||||<", 0};

BOOST_AUTO_TEST_CASE(TEST_split_strs) {
  using namespace std;
  {
    BOOST_CHECK_EQUAL(split_noquote(split_input, make_expect_visitor(split_chrs), ""), 7);
    BOOST_CHECK_EQUAL(split_noquote(split_input, make_expect_visitor(split_strs), ";"), 3);
    for (char const** p = split_seps; *p; ++p) {
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
} // namespace unit_test
#endif


} // namespace graehl

#endif
