/** \file

  char container with O(1) push_back.
*/

#ifndef STRING_BUFFER_GRAEHL_2015_10_29_HPP
#define STRING_BUFFER_GRAEHL_2015_10_29_HPP
#pragma once

#include <string>
#include <vector>
#include <graehl/shared/append.hpp>

namespace graehl {

#if __cplusplus >= 201103L
typedef std::string string_buffer;
#else
typedef std::vector<char> string_buffer;
#endif

#if __cplusplus >= 201103L
inline std::string const& str(graehl::string_buffer const& buf) {
  return buf;
}
#else
inline std::string str(graehl::string_buffer const& buf) {
  return std::string(buf.begin(), buf.end());
}
#endif
}

namespace std {
#if __cplusplus >= 201103L
#else
inline graehl::string_buffer& operator+=(graehl::string_buffer& buf, std::string const& str) {
  graehl::append(buf, str);
  return buf;
}
#endif
}

#endif
