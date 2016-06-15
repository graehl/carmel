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

    pre-c++11 shared_ptr, unique_ptr, reference_wrapper, function from boost,
    else from std
*/

#ifndef SHARED_PTR_GRAEHL_2015_10_24_HPP
#define SHARED_PTR_GRAEHL_2015_10_24_HPP
#pragma once
#include <graehl/shared/cpp11.hpp>

#if GRAEHL_CPP11
#include <memory>
#else
#include <boost/ref.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#endif


namespace graehl {

#if GRAEHL_CPP11
#define GRAEHL_SHARED_PTR_NS std
using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;
using std::static_pointer_cast;
using std::dynamic_pointer_cast;
using std::const_pointer_cast;
#else
#define GRAEHL_SHARED_PTR_NS boost
template <class T>
struct unique_ptr : boost::scoped_ptr<T> {};
using boost::shared_ptr;
using boost::make_shared;
using boost::static_pointer_cast;
using boost::dynamic_pointer_cast;
using boost::const_pointer_cast;
#endif


}

#endif
