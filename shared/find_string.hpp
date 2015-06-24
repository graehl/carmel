/** \file \author Jonathan Graehl <graehl@gmail.com>

    find_string(boost::unordered_map<std::string, ...>, pair<char const*, char
    const*>) pair is [begin, end), a key: map.find(std:string(key.first,
    key.second)) read-only since unordered_map doesn't support lazy construction
    of string from a pair key.

    To the extent possible under law, the author(s) have dedicated all copyright
    and related and neighboring rights to this software to the public domain
    worldwide. This software is distributed without any warranty.
*/

#ifndef FIND_STRING_GRAEHL_2015_06_24_HPP
#define FIND_STRING_GRAEHL_2015_06_24_HPP
#pragma once

#include <utility>
#include <algorithm>
#include <cstddef>
#include <boost/functional/hash.hpp>

namespace std {
/// we do not change standard semantics of any supported comparison e.g. pair vs
/// pair, but simply allow string to be compared against pair of char pointers.
inline bool operator==(std::string const& str, std::pair<char const*, char const*> slice) {
  return str.size() == (slice.second - slice.first) && std::equal(slice.first, slice.second, str.begin());
}
inline bool operator==(std::pair<char const*, char const*> slice, std::string const& str) {
  return str.size() == (slice.second - slice.first) && std::equal(slice.first, slice.second, str.begin());
}
inline bool operator==(std::string const& str, std::pair<char*, char*> slice) {
  return str.size() == (slice.second - slice.first) && std::equal(slice.first, slice.second, str.begin());
}
inline bool operator==(std::pair<char*, char*> slice, std::string const& str) {
  return str.size() == (slice.second - slice.first) && std::equal(slice.first, slice.second, str.begin());
}
/// techinically not allowed but easiest route to ADL. we could rename these instead.
inline std::size_t hash_value(std::pair<char const*, char const*> slice) {
  return boost::hash_range(slice.first, slice.second);
}
inline std::size_t hash_value(std::pair<char*, char*> slice) {
  return boost::hash_range(slice.first, slice.second);
}
inline std::size_t hash_value(std::string const& str) {
  return boost::hash_range(str.begin(), str.end());
}
}

struct slice_or_string_eq {
  typedef bool result_type;
  template <class A, class B>
  bool operator()(A const& a, B const& b) const {
    return a == b;
  }
};

struct slice_or_string_hash {
  typedef std::size_t result_type;
  template <class Slice>
  std::size_t operator()(Slice const& slice) const {
    return hash_value(slice);
  }
};

/// \return map.find(std:string(key.first, key.second)) but faster
template <class UnorderedMap, class Slice>
typename UnorderedMap::const_iterator find_string(UnorderedMap const& map, Slice const& key) {
  return map.find(key, slice_or_string_hash(), slice_or_string_eq());
}

/// \return map.find(std:string(key.first, key.second)) but faster
template <class UnorderedMap, class Slice>
typename UnorderedMap::iterator find_string(UnorderedMap& map, Slice const& key) {
  return map.find(key, slice_or_string_hash(), slice_or_string_eq());
}

#endif
