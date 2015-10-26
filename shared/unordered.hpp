/** \file

 .
*/

#ifndef UNORDERED_GRAEHL_2015_10_25_HPP
#define UNORDERED_GRAEHL_2015_10_25_HPP
#pragma once

// reminder: undefined macros eval to 0
#if __cplusplus >= 201103L || defined(__clang__) && defined(_LIBCPP_VERSION)
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

#endif
