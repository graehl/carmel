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

 bring unordered_map and _set into graehl ns.
*/

#ifndef UNORDERED_GRAEHL_2015_10_25_HPP
#define UNORDERED_GRAEHL_2015_10_25_HPP
#pragma once
#include <graehl/shared/cpp11.hpp>

// reminder: undefined macros eval to 0
#if GRAEHL_CPP11 || defined(__clang__) && defined(_LIBCPP_VERSION)
#include <unordered_map>
#include <unordered_set>
#define GRAEHL_UNORDERED_NS ::std
#elif _MSC_VER >= 1500 && _HAS_TR1 || _MSC_VER >= 1700
#include <unordered_map>
#include <unordered_set>
#define GRAEHL_UNORDERED_NS ::std::tr1
#elif __GNUC__ == 4 && __GNUC__MINOR__ >= 2
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#define GRAEHL_UNORDERED_NS ::std::tr1
#else
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#define GRAEHL_UNORDERED_NS ::boost
#endif

namespace graehl {
using GRAEHL_UNORDERED_NS::unordered_map;
using GRAEHL_UNORDERED_NS::unordered_set;


}

#endif
