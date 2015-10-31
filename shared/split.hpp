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
/** \file

    optimized implementations of common split-line-into-words-on-whitespace and
    related operations
*/

#ifndef GRAEHL_SHARED__SPLIT_HPP
#define GRAEHL_SHARED__SPLIT_HPP
#pragma once

#include <string>
#include <vector>
#include <utility>

namespace graehl {

template <class Strings, class Sep>
void join_append_to(std::string& r, Strings const& strs, Sep const& sep) {
  bool first = true;
  for (typename Strings::const_iterator i = strs.begin(), e = strs.end(); i != e; ++i) {
    if (first)
      first = false;
    else
      r += sep;
    r += *i;
  }
}

template <class Strings, class Sep>
void join_to(std::string& r, Strings const& strs, Sep const& sep) {
  r.clear();
  join_append_to(r, strs, sep);
}

template <class Strings, class Sep>
std::string joined(Strings const& strs, Sep const& sep) {
  std::string r;
  join_append_to(r, strs, sep);
  return r;
}

// TODO: default split on ' ' instead of ,? test.

inline bool split_first_rest(std::string const& s, std::string& first, std::string& rest,
                             std::string const& delim = ",") {
  using namespace std;
  string::size_type pos = s.find(delim);
  if (pos != string::npos) {
    first.assign(s, 0, pos);
    pos += delim.size();
    rest.assign(s, pos, s.size() - pos);
    return true;
  } else {
    first = s;
    rest.clear();
    return false;
  }
}

inline bool split_rest_last(std::string const& s, std::string& rest, std::string& last,
                            std::string const& delim = ",") {
  using namespace std;
  string::size_type pos = s.rfind(delim);
  if (pos != string::npos) {
    rest.assign(s, 0, pos);
    pos += delim.size();
    last.assign(s, pos, s.size() - pos);
    return true;
  } else {
    rest = s;
    last.clear();
    return false;
  }
}

template <class Cont>
struct split_push_back {
  Cont* c;
  split_push_back(Cont& cr) : c(&cr) {}
  template <class Str>
  bool operator()(Str const& s) {
    c->push_back(s);
    return true;
  }
};

template <class Cont, class Delimiter>
void split_into(Cont& r, std::string const& str, Delimiter const& delimiter) {
  std::string::size_type start = 0, end;
  while ((end = str.find(delimiter, start)) != std::string::npos) {
    r.push_back(std::string(str, start, end - start));
    start = end + 1;
  }
  r.push_back(std::string(str, start));
}

template <class Visit, class Delimiter>
void split_visit(Visit const& f, std::string const& str, Delimiter const& delimiter) {
  std::string::size_type start = 0, end;
  while ((end = str.find(delimiter, start)) != std::string::npos) {
    f(std::string(str, start, end - start));
    start = end + 1;
  }
  f(std::string(str, start));
}

template <class Cont, class Chars>
void split_into_any(Cont& r, std::string const& str, Chars const& chars) {
  std::string::size_type start = 0, end;
  while ((end = str.find_first_of(chars, start)) != std::string::npos) {
    r.push_back(std::string(str, start, end - start));
    start = end + 1;
  }
  r.push_back(std::string(str, start));
}

template <class Visit, class Chars>
void split_visit_any(Visit const& f, std::string const& str, Chars const& chars) {
  std::string::size_type start = 0, end;
  while ((end = str.find_first_of(chars, start)) != std::string::npos) {
    f(std::string(str, start, end - start));
    start = end + 1;
  }
  f(std::string(str, start));
}

template <class Cont>
void split_into(Cont& r, std::string const& str) {
  split_into(r, str, ',');
}


template <class Delimiter>
inline std::vector<std::string> split(std::string const& str, Delimiter const& delim) {
  std::vector<std::string> c;
  split_into(c, str, delim);
  return c;
}

template <class Chars>
inline std::vector<std::string> split_any(std::string const& str, Chars const& chars) {
  std::vector<std::string> c;
  split_into(c, str, chars);
  return c;
}

template <class Delimiter>
inline std::vector<std::string> split(std::string const& str) {
  std::vector<std::string> c;
  split_into(c, str, ' ');
  return c;
}

template <class Cont>
void chomped_lines_into(Cont& r, std::string const& str) {
  std::string::size_type start = 0, end, len = str.size();
  char const* data = str.data();
  while ((end = str.find('\n', start)) != std::string::npos) {
    char const* last = data + end - 1;
    r.push_back(std::string(data + start, *last == '\r' ? last : last + 1));
    start = end + 1;
  }
  if (start < end) {
    char const* last = data + len - 1;
    r.push_back(std::string(data + start, *last == '\r' ? last : last + 1));
  }
}

template <class Cont>
void chomped_lines_into(Cont& r, std::istream& in) {
  std::string line;
  while (std::getline(in, line)) {
    std::string::size_type len = line.size();
    if (len && line[--len] == '\r') line.resize(len);
#if __cplusplus >= 201103L
    r.push_back(std::move(line));
#else
    r.push_back(line);
#endif
  }
}

inline std::vector<std::string> chomped_lines(std::string const& str) {
  std::vector<std::string> r;
  chomped_lines_into(r, str);
  return r;
}

inline std::vector<std::string> chomped_lines(std::istream& in) {
  std::vector<std::string> r;
  chomped_lines_into(r, in);
  return r;
}


}

#endif
