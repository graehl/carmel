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

 std:: enable_if etc or boost::.
*/

#ifndef TYPE_TRAITS_GRAEHL_2015_10_28_HPP
#define TYPE_TRAITS_GRAEHL_2015_10_28_HPP
#pragma once
#include <graehl/shared/cpp11.hpp>

#if GRAEHL_CPP11
#define GRAEHL_TYPE_TRAITS_NS std
#include <type_traits>
#else
#define GRAEHL_TYPE_TRAITS_NS boost
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/is_lvalue_reference.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_fundamental.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/mpl/bool_fwd.hpp>
#include <boost/type_traits/integral_constant.hpp>
#endif

#include <boost/icl/type_traits/is_container.hpp>

namespace graehl {
using GRAEHL_TYPE_TRAITS_NS::remove_cv;
using GRAEHL_TYPE_TRAITS_NS::remove_reference;
using GRAEHL_TYPE_TRAITS_NS::remove_pointer;

using GRAEHL_TYPE_TRAITS_NS::is_lvalue_reference;
using GRAEHL_TYPE_TRAITS_NS::is_integral;
using GRAEHL_TYPE_TRAITS_NS::is_arithmetic;
using GRAEHL_TYPE_TRAITS_NS::is_fundamental;
using GRAEHL_TYPE_TRAITS_NS::is_enum;
using GRAEHL_TYPE_TRAITS_NS::is_class;
using GRAEHL_TYPE_TRAITS_NS::is_pointer;
using GRAEHL_TYPE_TRAITS_NS::is_base_of;
using boost::icl::is_container;

#if GRAEHL_CPP11
template <class T>
struct identity { using type = T; };
template <class T>
using not_deducible = typename identity<T>::type;
template <bool B, typename...>
struct dependent_bool_type : std::integral_constant<bool, B> {};
/// usage: static_assert(Bool<false, T>::value, "T concept error msg");
template <bool B, typename... T>
using Bool = typename dependent_bool_type<B, T...>::type;

using std::enable_if;
template <class A, class B>
using disable_if_same_or_derived =
    typename enable_if<!is_base_of<A, typename remove_reference<B>::type>::value>::type;

using std::true_type;
using std::false_type;
using std::integral_constant;
template <bool B, class T = void>
using disable_if = enable_if<!B, T>;
#else
template <bool B, class T = void>
struct enable_if : boost::enable_if_c<B, T> {};
template <bool B, class T = void>
struct disable_if : boost::enable_if_c<!B, T> {};
typedef boost::mpl::true_ true_type;
typedef boost::mpl::false_ false_type;
using boost::integral_constant;
#endif


}

#endif
