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

    pre-c++11 function, reference_wrapper (MSVC 2015 bug: force boost::function)
*/

#ifndef FUNCTION_GRAEHL_2015_10_24_HPP
#define FUNCTION_GRAEHL_2015_10_24_HPP
#pragma once
#include <graehl/shared/cpp11.hpp>

#if GRAEHL_CPP11
#include <functional>
#else
#include <boost/ref.hpp>
#endif

#if !GRAEHL_CPP11 || (defined(_MSC_VER) && _MSC_VER <= 1900)
#define GRAEHL_FUNCTION_NS boost
#include <boost/function.hpp>
#else
#include <functional>
#define GRAEHL_FUNCTION_NS std
#endif

namespace graehl {

#if GRAEHL_CPP11
using std::ref;
using std::reference_wrapper;
template <class T>
struct unwrap_reference {
  typedef T type;
};
template <class T>
struct unwrap_reference<reference_wrapper<T>> {
  typedef T type;
};
#else
using boost::ref;
using boost::reference_wrapper;
using boost::unwrap_reference;
#endif

using GRAEHL_FUNCTION_NS::function;


}

#endif
