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

    container (e.g. vector) of things from space-separated string or vector of
    strings.
*/


#ifndef FROM_STRINGS_JG201266_HPP
#define FROM_STRINGS_JG201266_HPP
#pragma once

#include <graehl/shared/adl_to_string.hpp>
#include <graehl/shared/container.hpp>
#include <graehl/shared/is_container.hpp>
#include <graehl/shared/pointer_traits.hpp>
#include <graehl/shared/string_buffer.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/type_traits.hpp>
#include <stdexcept>

namespace graehl {

template <class Val, class enable = void>
struct select_from_strings {
  enum { container = 0 };
  typedef std::string string_or_strings;
  typedef std::vector<std::string> strings_type;
  typedef Val value_type;
  static inline void from_strings(std::vector<std::string> const& s, Val& val) {
    if (s.size() == 1)
      string_to(s.front(), val);
    else
      throw std::runtime_error("from_strings: non-container value expected exactly one source string");
  }
  static inline std::vector<std::string> to_strings(Val const& val) {
    return strings_type(1, adl::adl_to_string(val));
  }
  static inline std::string to_string(Val const& val, std::string const& sep_each, std::string pre_each) {
    adl::adl_append_to_string(pre_each, val);
    return pre_each;
  }
  static inline std::string to_string(Val const& val, std::string const& sep_each = " ") {
    return adl::adl_to_string(val);
  }
  static inline string_or_strings to_string_or_strings(Val const& val) { return to_string(val); }
};


template <class Container>
inline std::string range_to_string(Container const& container, std::string const& sep_each = " ",
                                   std::string const& pre_each = "") {
  string_buffer o;
  o.reserve(20 * container.size());
  bool first = true;
  for (typename Container::const_iterator i = container.begin(), e = container.end(); i != e; ++i) {
    if (first)
      first = false;
    else
      append(o, sep_each);
    append(o, pre_each);
    adl::adl_append_to_string(o, *i);
  }
  return str(o);
}

template <class Container>
struct select_from_strings<Container, typename enable_if<is_nonstring_container<Container>::value>::type> {
  enum { container = 1 };
  typedef std::vector<std::string> strings_type;
  typedef strings_type string_or_strings;
  typedef typename Container::value_type value_type;
  static inline void from_strings(std::vector<std::string> const& s, Container& container) {
    container.clear();
    for (typename std::vector<std::string>::const_iterator i = s.begin(), e = s.end(); i != e; ++i)
      add(container, string_to<value_type>(*i));
  }
  static inline std::vector<std::string> to_strings(Container const& container) {
    // TODO: test
    strings_type r;
    for (typename Container::const_iterator i = container.begin(), e = container.end(); i != e; ++i)
      add(r, adl::adl_to_string(*i));
    return r;
  }
  static inline std::string to_string(Container const& container, std::string const& sep_each = " ",
                                      std::string const& pre_each = "") {
    return range_to_string(container, sep_each, pre_each);
  }
  static inline string_or_strings to_string_or_strings(Container const& container) {
    return to_strings(container);
  }
};

template <class V>
inline typename select_from_strings<V>::string_or_strings to_string_or_strings(V const& v) {
  return select_from_strings<V>::to_string_or_strings(v);
}

template <class V>
inline std::string to_string_sep(V const& v, std::string const& sep_each = " ",
                                 std::string const& pre_each = "") {
  return select_from_strings<V>::to_string(v, sep_each, pre_each);
}


struct store_from_strings {
  virtual void from_strings(std::vector<std::string> const& s) = 0;
  virtual std::string to_string(std::string const& pre_each = "", std::string const& sep_each = " ") const = 0;
  virtual bool container() const = 0;
};

template <class Ptr, class enable = void>
struct ptr_from_strings : store_from_strings {
  Ptr p;
  typedef typename pointer_traits<Ptr>::element_type value_type;
  ptr_from_strings(Ptr const& p = Ptr()) : p(p) {}
  ptr_from_strings(ptr_from_strings const& o) : p(o.p) {}
  void ensure_ptr() {
    if (!p) p = Ptr(new value_type());
  }
  void from_strings(std::vector<std::string> const& s) {
    ensure_ptr();
    select_from_strings<value_type>::from_strings(s, *p);
  }
  std::string to_string(std::string const& pre_each = "", std::string const& sep_each = " ") const {
    ensure_ptr();
    return select_from_strings<value_type>::to_string(*p, pre_each, sep_each);
  }
  bool container() const { return select_from_strings<value_type>::container; }
};


}

#endif
