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

 append to vector, C++98 string (missing insert(end(), from, to), etc.

*/
#ifndef APPEND_JG_2015_01_07_HPP
#define APPEND_JG_2015_01_07_HPP
#pragma once

namespace graehl {

template <class V, class Iter>
void append(V& v, Iter begin, Iter end) {
  v.insert(v.end(), begin, end);
}

template <class Iter>
void append(std::string& v, Iter begin, Iter end) {
  v.append(begin, end);
}

inline void append(std::string& s, std::string const& x) {
  s.append(x);
}

template <class C, class Range>
inline void append(C& c, Range const& range) {
  append(c, range.begin(), range.end());
}


}

#endif
